#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// --- SUAS CREDENCIAIS DE WIFI ---
const char SSID[] = "AlexandreCarvalho";        // Nome da rede confirmado no seu scan
const char PASS[] = "teste123"; // <--- DIGITE SUA SENHA DENTRO DAS ASPAS

// Declaração das funções que serão chamadas quando as variáveis mudarem
void onOpenMsChange();
void onDoorCmdChange();

// Suas variáveis da Cloud
String lastEvent;
int openMs;
bool doorCmd;
bool doorState;

// Inicialização das propriedades
void initProperties(){
  // Como sua placa é R4 WiFi, a autenticação segura já deve estar configurada no chip.
  // Se der erro de "Device ID", avise-me.

  ArduinoCloud.addProperty(lastEvent, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(openMs, READWRITE, ON_CHANGE, onOpenMsChange, 5000);
  ArduinoCloud.addProperty(doorCmd, READWRITE, ON_CHANGE, onDoorCmdChange);
  ArduinoCloud.addProperty(doorState, READ, ON_CHANGE, NULL);
}

// Configura a conexão usando o SSID e SENHA que escrevemos acima
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
