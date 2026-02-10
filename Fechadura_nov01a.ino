#include "thingProperties.h"
#include <Adafruit_Fingerprint.h>
#include <SPI.h>       
#include <MFRC522.h>   
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;  // Autodeteção do endereço I2C

// ======== PINAGEM RELÉ ===========
#define RELAY_PIN 4       // Pino do Relé (Tranca)
#define BUTTON_PIN 2      // Pino do Botão de Saída 
#define RELAY_ACTIVE_HIGH true

// ======== CONFIGURAÇÃO RFID (RC522) ===========
#define SS_PIN 10  // Pino SDA
#define RST_PIN 9  // Pino RST
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ID do Cartão Mestre (Autorizado)
const String MASTER_CARD_UID = "6E E7 3C 06"; 

// ======== SENSOR BIOMÉTRICO ===========
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

// ======== VARIÁVEIS GERAIS ===========
unsigned long doorOpenStart = 0;
bool doorOpen = false;

// ==========================================
//                FUNÇÕES AUXILIARES
// ==========================================

void setRelay(bool open) {
  if (RELAY_ACTIVE_HIGH)
    digitalWrite(RELAY_PIN, open ? HIGH : LOW);
  else
    digitalWrite(RELAY_PIN, open ? LOW : HIGH);
}

void openDoorNow() {
  setRelay(true);
  doorOpen = true;
  doorState = true; // Variável da Cloud
  doorOpenStart = millis();
  lastEvent = "Fechadura ABERTA"; // Variável da Cloud
  ArduinoCloud.update();

  Serial.println("[LOG] Fechadura aberta");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Acesso Permitido");
  lcd.setCursor(0, 1); lcd.print("BEM-VINDO!");
}

void closeDoorNow() {
  setRelay(false);
  doorOpen = false;
  doorState = false; // Variável da Cloud
  lastEvent = "Fechadura FECHADA"; // Variável da Cloud
  ArduinoCloud.update();

  Serial.println("[LOG] Fechadura fechada");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Cartao / Digital");
  lcd.setCursor(0, 1); lcd.print(" >> Aproxime << ");
}

// ==========================================
//          FUNÇÃO DE LEITURA BIOMÉTRICA
// ==========================================
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  Serial.print("[LOG] Digital ID: ");
  Serial.println(finger.fingerID);

  lcd.clear();
  lcd.print("Digital OK ID ");
  lcd.print(finger.fingerID);

  return FINGERPRINT_OK;
}

// ==========================================
//                  SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
   
  // Configura Relé
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false); // Garante que começa fechada

  // Configura Botão
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (openMs == 0) openMs = 5000;

  // === Iniciar LCD ===
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Inicializando...");

  // === Iniciar Cloud ===
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  lcd.setCursor(0, 1);
  lcd.print("WiFi a ligar...");
   
  // === Iniciar Biometria ===
  Serial1.begin(57600);
  finger.begin(57600);

  // --- ALTERAÇÃO SOLICITADA ---
  if (finger.verifyPassword()) {
    Serial.println("[INFO] Sensor biométrico OK");
  } else {
    Serial.println("[ERRO] Sensor biométrico falhou!");
    
    // Mostra mensagem de erro no LCD
    lcd.clear(); 
    lcd.setCursor(0, 0); lcd.print("Fora de Servico");
    lcd.setCursor(0, 1); lcd.print("Erro Tecnico");
    
    // BLOQUEIA O SISTEMA AQUI
    while (true) {
      delay(1000); 
    }
  }
  // -----------------------------

  // === Iniciar RFID ===
  SPI.begin();           // Inicia barramento SPI
  mfrc522.PCD_Init();  // Inicia o módulo RFID
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); // Máxima sensibilidade
  Serial.println("[INFO] Sensor RFID pronto");

  delay(1500);
  closeDoorNow(); // Vai para o ecrã inicial
}

// ==========================================
//                    LOOP
// ==========================================
void loop() {
  ArduinoCloud.update();

  // 1. Fecho automático (Temporizador)
  if (doorOpen && millis() - doorOpenStart >= (unsigned long)openMs) {
    closeDoorNow();
  }

  // 2. Botão de Saída 
  if (!doorOpen && digitalRead(BUTTON_PIN) == LOW) {
      lastEvent = "Saida Manual";
      openDoorNow();
      delay(500); // Evita duplo clique (debounce)
  }

  // 3. Biometria (Verifica a cada 100ms)
  static unsigned long lastFP = 0;
  if (millis() - lastFP > 100) {
    lastFP = millis();
    if (getFingerprintID() == FINGERPRINT_OK) {
      lastEvent = "Acesso DIGITAL";
      openDoorNow();
    }
  }

  // 4. Leitura RFID 
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      
    // Converte ID lido para String legível
    String readUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      readUID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      readUID.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    readUID.toUpperCase();
    readUID = readUID.substring(1); // Remove espaço inicial

    Serial.print("[RFID] Lido: "); Serial.println(readUID);

    // Verifica se é o cartão mestre
    if (readUID == MASTER_CARD_UID) {
      lastEvent = "Acesso CARTAO";
      openDoorNow();
    } else {
      lastEvent = "Cartao NEGADO";
      ArduinoCloud.update();
        
      lcd.clear();
      lcd.print("Cartao Invalido");
      lcd.setCursor(0,1); lcd.print(readUID); // Mostra ID para debug
        
      delay(2000);
      if (!doorOpen) closeDoorNow();
    }
      
    // Para a leitura para não repetir
    mfrc522.PICC_HaltA(); 
    mfrc522.PCD_StopCrypto1();
  }
}

// ==========================================
//          CALLBACKS IOT CLOUD
// ==========================================
void onDoorCmdChange() {
  if (doorCmd) openDoorNow();
  else closeDoorNow();
}

void onOpenMsChange() {
  if (openMs < 1000) openMs = 1000;
  if (openMs > 30000) openMs = 30000;
}
