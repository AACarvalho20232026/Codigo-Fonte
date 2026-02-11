#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// ---  CREDENCIAIS DE WIFI ---
const char SSID[] = "AlexandreCarvalho";        
const char PASS[] = "teste123"; 

// Declaração das funções que serão chamadas quando as variáveis mudarem
void onOpenMsChange();
void onDoorCmdChange();

//  variáveis da Cloud
String lastEvent;
int openMs;
bool doorCmd;
bool doorState;

// Inicialização das propriedades
void initProperties(){
  ArduinoCloud.addProperty(lastEvent, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(openMs, READWRITE, ON_CHANGE, onOpenMsChange, 5000);
  ArduinoCloud.addProperty(doorCmd, READWRITE, ON_CHANGE, onDoorCmdChange);
  ArduinoCloud.addProperty(doorState, READ, ON_CHANGE, NULL);
}

// Configura a conexão usando o SSID e SENHA que escrevemos acima
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
