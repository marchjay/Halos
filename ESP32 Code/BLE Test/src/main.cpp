#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>

BLEServer* pServer = nullptr;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      deviceConnected = true;
      Serial.println("[BLE] Device connected.");
    }

    void onDisconnect(BLEServer* pServer) override {
      deviceConnected = false;
      Serial.println("[BLE] Device disconnected.");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE setup...");

  // Initialize BLE device
  BLEDevice::init("HalosBeam-BLE");

  // Create BLE server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x180F));  // Battery Service UUID (as example)
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // recommended defaults
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();
  Serial.println("[BLE] Advertising started as 'HalosBeam-BLE'");
}

void loop() {
  // Add any other debug or status checks here
  delay(1000);
}