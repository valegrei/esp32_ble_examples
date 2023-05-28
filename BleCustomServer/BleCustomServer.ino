/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example inputValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "EEPROM.h"

int addr = 0;
#define EEPROM_SIZE 64

BLEServer *pServer = NULL;
BLECharacteristic *infoCharacteristic;
BLECharacteristic *resCharacteristic;
BLECharacteristic *statusCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
std::string dataReceived = "";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// Servicios
#define DATA_SERVICE_UUID "49e0b347-e722-4ac0-92fb-a316e887fdea"  // Leer y escribir 
#define SENSOR_SERVICE_UUID "5240682c-f85b-455c-ba69-2cadcefbabca" // Sensores y estados en tiempo real 

// Caracteristicas
#define CHARACTERISTIC_INFO_UUID "bee6d633-72e2-4336-9199-faa484d14b95" // Lee metadatos del equipo
#define CHARACTERISTIC_REQ_UUID "52491fd0-7498-4cd5-9cd6-b03c1d3b5272" // Recibe comandos
#define CHARACTERISTIC_RES_UUID "64f316dc-0a57-49f0-9f48-9500a9009d93" // Responde a comandos
#define CHARACTERISTIC_STATUS_UUID "a12f4c9e-503b-45eb-8d7b-7fb774cf51d1" // Notifica el estado de los sensores a tiempo real

/* This function handles the server callbacks */
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void printDataReceived() {
  if(dataReceived == "H-L"){
    int f = readHour(1);
    std::string msg1 = "H-L-ABCDEF";
    std::string msg2 = "H-M-ABCDEF";
    std::string msg3 = "H-K-ABCDEF";
    std::string msg4 = "H-J-ABCDEF";
    std::string msg5 = "H-V-ABCDEF";
    std::string msg6 = "H-S-ABCDEF";
    std::string msg7 = "H-D-ABCDEF";
    std::string F = std::to_string(f);
    resCharacteristic->setValue(msg1);
    resCharacteristic->indicate();
  }
  if(dataReceived == "ON"){
    resCharacteristic->setValue("ON");
    resCharacteristic->indicate();
    relay("ON");
    Serial.println("ON RECIBIDO");
  }
    if(dataReceived == "OFF"){
    resCharacteristic->setValue("OFF");
    resCharacteristic->indicate();
    relay("OFF");
    Serial.println("OFF RECIBIDO");
  }// Serial.println("*********");
  // Serial.print("Received Value: ");
  // Serial.println(dataReceived);
  // Serial.println("*********");
  //resCharacteristic->setValue(dataReceived);
  //resCharacteristic->notify();
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string inputValue = pCharacteristic->getValue();

    if (inputValue.length() > 0) {
      for (int i = 0; i < inputValue.length(); i++) {
        if (inputValue[i] == '|') {
          printDataReceived();
          dataReceived = "";
          break;
        } else {
          dataReceived = dataReceived + inputValue[i];
        }
      }
    }
  }
  
};



void setup() {
  Serial.begin(115200);
  EEPROMBEGIN();
  pinMode(14,OUTPUT);
  pinMode(33,OUTPUT);
  digitalWrite(33,HIGH);
  // Create and name the BLE Device
  BLEDevice::init("Biosfera Biologica");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // Set the function that handles Server Callbacks

  // Create the BLE Service
  BLEService *dataService = pServer->createService(DATA_SERVICE_UUID);
  BLEService *sensorService = pServer->createService(SENSOR_SERVICE_UUID);

  // Create a BLE Characteristic
  infoCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_INFO_UUID,
    BLECharacteristic::PROPERTY_READ);

  BLECharacteristic *reqCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_REQ_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  reqCharacteristic->setCallbacks(new MyCallbacks());

  resCharacteristic = dataService->createCharacteristic(
    CHARACTERISTIC_RES_UUID,
    BLECharacteristic::PROPERTY_INDICATE);//PROPERTY_NOTIFY

  resCharacteristic->addDescriptor(new BLE2902());

  statusCharacteristic = sensorService->createCharacteristic(
    CHARACTERISTIC_STATUS_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  statusCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  dataService->start();
  sensorService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  // Aqui recolectas los datos de los sensores y enviarlo con setValue, luego notificar
  if (deviceConnected) {
    int Battery = map(analogRead(34)*3.30/2048.00,0,3.20,0,100);
    Serial.println(Battery);
    statusCharacteristic->setValue(Battery);
    statusCharacteristic->notify();
    //txValue++;
    delay(10);  // bluetooth stack will go into congestion, if too many packets are sent
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


void EEPROMBEGIN(){
  
  Serial.println("start...");
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }
  Serial.println();
  Serial.println("writing random n. in memory");

}

uint8_t readHour(int addres){
  uint8_t value = EEPROM.read(addres);
  return value; 
}
void relay(String s){
  if(s=="ON"){
    digitalWrite(33,HIGH);
  }
  if(s=="OFF"){
    digitalWrite(33,LOW);
  }

}