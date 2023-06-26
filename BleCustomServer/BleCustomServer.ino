#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "EEPROM.h"
#include <ArduinoJson.h>

// Servicios
#define DATA_SERVICE_UUID "49e0b347-e722-4ac0-92fb-a316e887fdea"  // Leer y escribir

// Caracteristicas
#define CHARACTERISTIC_REQ_UUID "52491fd0-7498-4cd5-9cd6-b03c1d3b5272"     // Recibe comandos
#define CHARACTERISTIC_RES_UUID "64f316dc-0a57-49f0-9f48-9500a9009d93"     // Responde a comandos
#define CHARACTERISTIC_STATUS_UUID "a12f4c9e-503b-45eb-8d7b-7fb774cf51d1"  // Notifica el estado de los sensores a tiempo real

#define U_TYPE_ADMIN 1
#define U_TYPE_TECNICO 2
#define U_TYPE_USUARIO 3

#define CMD_LOGIN 1
#define CMD_RELAY_INFO 2
#define CMD_RELAY_UPDATE 3
#define CMD_CONTROL 4
#define CMD_CHANGE_PASS 5
#define CMD_WIFI_INFO 6
#define CMD_CHANGE_WIFI 7

#define EEPROM_SIZE 64
#define MTU_SIZE 517

int addr = 0;

BLEServer *pServer = NULL;
BLECharacteristic *resCharacteristic;
BLECharacteristic *statusCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
std::string receivedData = "";
const char *admin_pass = "admin";
const char *tecnico_pass = "tecnico";
const char *usu_pass = "usuario";


/* This function handles the server callbacks */
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

/*
* Envia json como respuesta
*/
void writeJson(BLECharacteristic *characteristic, DynamicJsonDocument &outputJson){
  std::string jsonString;
  serializeJson(outputJson, jsonString);
  writeLargeText(characteristic, jsonString);
}

/*
* Envia cadena como respuesta
*/
void writeLargeText(BLECharacteristic *characteristic, std::string largeText) {
  for (int i = 0; i < largeText.length(); i += MTU_SIZE - 3) {
    int len = MTU_SIZE - 3;
    if (len > largeText.length() - i)
      len = largeText.length() - i;
    characteristic->setValue(largeText.substr(i, len));
    characteristic->notify();
  }
  characteristic->setValue("|");
  characteristic->notify();
}

/**
 * Inicio de Sesion
 * 
 * @param req Json con los dato de solicitud
 */
void handleLogin(DynamicJsonDocument &req) {
  int u_type = req["u_type"];
  const char *u_pass = req["u_pass"];
  const char *pass;
  switch (u_type) {
    case U_TYPE_ADMIN:
      pass = admin_pass;
      break;
    case U_TYPE_TECNICO:
      pass = tecnico_pass;
      break;
    case U_TYPE_USUARIO:
      pass = usu_pass;
      break;
    default:
      break;
  }
  DynamicJsonDocument res(100);
  res["cmd"] = CMD_LOGIN;
  if (strcmp(u_pass,pass)==0) {  //EXITO
    res["success"] = true;
    res["n_serie"] = "123456789";
    res["version"] = 1.0;
    res["relays"] = 3;
  } else {
    res["success"] = false;
  }
  writeJson(resCharacteristic, res);
}

void handleRelayInfo(DynamicJsonDocument &req){
  // TODO: implementar
  int id = req["id"];
  switch(id){
    case 1:
      writeLargeText(resCharacteristic, "{\"cmd\":2,\"success\":true,\"id\":1,\"name\":\"motor 1\",\"t_on\":100,\"dias\":[1,0,1,0,1,0,0],\"h_dia\":6,\"h_l\":[10,11,12,13,14,15],\"h_m\":[10,11,12,13,14,15],\"h_k\":[10,11,12,13,14,15],\"h_j\":[10,11,12,13,14,15],\"h_v\":[10,11,12,13,14,15],\"h_s\":[10,11,12,13,14,15],\"h_d\":[10,11,12,13,14,15]}");
      break;
    case 2:
      writeLargeText(resCharacteristic, "{\"cmd\":2,\"success\":true,\"id\":2,\"name\":\"relay 2\",\"t_on\":50,\"dias\":[1,1,1,1,1,0,0],\"h_dia\":3,\"h_l\":[10,11,12,13,14,15],\"h_m\":[10,11,12,13,14,15],\"h_k\":[10,11,12,13,14,15],\"h_j\":[10,11,12,13,14,15],\"h_v\":[10,11,12,13,14,15],\"h_s\":[10,11,12,13,14,15],\"h_d\":[10,11,12,13,14,15]}");
      break;
    case 3:
      writeLargeText(resCharacteristic, "{\"cmd\":2,\"success\":true,\"id\":3,\"name\":\"valvula 3\",\"t_on\":20,\"dias\":[1,1,1,1,1,1,1],\"h_dia\":2,\"h_l\":[10,11,12,13,14,15],\"h_m\":[10,11,12,13,14,15],\"h_k\":[10,11,12,13,14,15],\"h_j\":[10,11,12,13,14,15],\"h_v\":[10,11,12,13,14,15],\"h_s\":[10,11,12,13,14,15],\"h_d\":[10,11,12,13,14,15]}");
      break;
  }
}

void handleReceivedData() {
  DynamicJsonDocument req(1024);
  deserializeJson(req, receivedData);
  int cmd = req["cmd"];
  switch (cmd) {
    case CMD_LOGIN:
      handleLogin(req);
      break;
    case CMD_RELAY_INFO:
      handleRelayInfo(req);
      break;
    case CMD_RELAY_UPDATE:

      break;
    case CMD_CONTROL:

      break;
    case CMD_CHANGE_PASS:

      break;
    case CMD_WIFI_INFO:

      break;
    case CMD_CHANGE_WIFI:

      break;
    default:

      break;
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string inputValue = pCharacteristic->getValue();

    if (inputValue.length() > 0) {
      for (int i = 0; i < inputValue.length(); i++) {
        if (inputValue[i] == '|') {
          handleReceivedData();
          receivedData = "";
          break;
        } else {
          receivedData = receivedData + inputValue[i];
        }
      }
    }
  }
};

void leerSensores(){
  DynamicJsonDocument res(1024);
  int i = 0;
  for(int j=1; j <= 3; j++){
    res["data"][i]["id"] = j;
    res["data"][i]["s_type"] = 1; // Relay
    res["data"][i]["name"] = "Relay";
    res["data"][i]["status"] = true;
    i++;
  }
  // Temperatura
  res["data"][i]["id"] = 1;
  res["data"][i]["s_type"] = 2; // Sensor Temperatura
  res["data"][i]["status"] = true;
  res["data"][i]["value"] = 20;
  i++;
  // Boya
  res["data"][i]["id"] = 2;
  res["data"][i]["s_type"] = 3; // Sensor Boya
  res["data"][i]["status"] = true;
  res["data"][i]["value"] = 25;
  i++;
  // Bateria
  res["data"][i]["id"] = 2;
  res["data"][i]["s_type"] = 4; // bateria
  res["data"][i]["status"] = true;
  res["data"][i]["value"] = 75;
  i++;

  //Enviar
  writeJson(statusCharacteristic, res);
}

void setup() {
  Serial.begin(115200);
  EEPROMBEGIN();
  pinMode(14, OUTPUT);
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  // Create and name the BLE Device
  BLEDevice::init("Biosfera Biologica");
  BLEDevice::setMTU(MTU_SIZE);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // Set the function that handles Server Callbacks

  // Create the BLE Service
  BLEService *dataService = pServer->createService(DATA_SERVICE_UUID);

  // Create a BLE Characteristic
  BLECharacteristic *reqCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_REQ_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  reqCharacteristic->setCallbacks(new MyCallbacks());

  resCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_RES_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);  //PROPERTY_NOTIFY

  resCharacteristic->addDescriptor(new BLE2902());

  statusCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_STATUS_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  statusCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  dataService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  // Aqui recolectas los datos de los sensores y enviarlo con setValue, luego notificar
  if (deviceConnected) {
    leerSensores();
    //txValue++;
    delay(100);  // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}


void EEPROMBEGIN() {

  Serial.println("start...");
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to initialise EEPROM");
    delay(1000000);
  }
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print(byte(EEPROM.read(i)));
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("writing random n. in memory");
}

uint8_t readHour(int addres) {
  uint8_t value = EEPROM.read(addres);
  return value;
}
void relay(String s) {
  if (s == "ON") {
    digitalWrite(33, HIGH);
  }
  if (s == "OFF") {
    digitalWrite(33, LOW);
  }
}