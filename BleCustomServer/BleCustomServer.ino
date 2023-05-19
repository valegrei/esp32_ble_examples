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

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
String dataReceived = "";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define DATA_SERVICE_UUID "bd9ab0ee-b957-4f64-8a1f-c3e7dd46fab4"  // Servicio para leer y escribir 
#define SENSOR_SERVICE_UUID "44a28349-14d8-4c84-af39-a656e00adc9f" // Servicio para sensores y estados en tiempo real 
#define CHARACTERISTIC_INFO_UUID "56fa711f-5b89-4109-8db7-52b2f731e013" // Caracteristica para leer metadatos del equipo
#define CHARACTERISTIC_STATUS_UUID "5f3a7010-9f20-4b53-88c4-14a0fdc95f06"
#define CHARACTERISTIC_STATUS_UUID "5f3a7010-9f20-4b53-88c4-14a0fdc95f06"
#define CHARACTERISTIC_STATUS_UUID "5f3a7010-9f20-4b53-88c4-14a0fdc95f06"

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
  Serial.println("*********");
  Serial.print("Received Value: ");
  Serial.println(dataReceived);
  Serial.println("*********");
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      //Serial.println("*********");
      //Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        //   Serial.print(rxValue[i]);
        if (rxValue[i] == '|') {
          printDataReceived();
          dataReceived = "";
        } else {
          dataReceived = dataReceived + rxValue[i];
        }
      }
      // Serial.println();
      //Serial.println("*********");
    }
  }
};



void setup() {
  Serial.begin(115200);

  // Create and name the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // Set the function that handles Server Callbacks

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  // Aqui recolectas los datos de los sensores y enviarlo con setValue, luego notificar
  if (deviceConnected) {
    pTxCharacteristic->setValue(&txValue, 1);
    pTxCharacteristic->notify();
    txValue++;
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
