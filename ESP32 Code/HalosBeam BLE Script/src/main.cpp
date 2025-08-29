#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// LED Pins
#define mainLight 21
#define leftSignal 33
#define rightSignal 32

// Main Light State
bool mainLightState = false;
uint8_t brightnessLvl[] = {50, 127, 255};
uint8_t brightnessIndex = 0;

// Left Signal State
bool leftIsFlashing = false;
bool leftLightState = LOW;
unsigned long leftLastFlashToggle = 0;

// Right Signal State
bool rightIsFlashing = false;
bool rightLightState = LOW;
unsigned long rightLastFlashToggle = 0;

const unsigned long flashInterval = 400;

// BLE UUIDs
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef12-3456-7890-abcd-ef1234567890"

// Connection status tracking
bool bleConnected = false;

// Global BLE characteristic pointer for notifications
BLECharacteristic* commandCharacteristic;

// BLE Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    bleConnected = true;
    Serial.println("✅ BLE client connected!");
  }

  void onDisconnect(BLEServer* pServer) override {
    bleConnected = false;
    Serial.println("❌ BLE client disconnected!");
    BLEDevice::startAdvertising();  // keep advertising
  }
};

// BLE Write Handler
class BLECommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    Serial.print("BLE Command Received: ");
    Serial.println(value.c_str());

    if (value == "TOGGLE") {
      mainLightState = !mainLightState;
      if (mainLightState) {
        analogWrite(mainLight, brightnessLvl[brightnessIndex]);
        Serial.println("Main light ON");
        std::string stateMsg = mainLightState ? "STATE:ON" : "STATE:OFF";
        commandCharacteristic->setValue(stateMsg);
        commandCharacteristic->notify();
      } else {
        analogWrite(mainLight, 0);
        Serial.println("Main light OFF");
        std::string stateMsg = mainLightState ? "STATE:ON" : "STATE:OFF";
        commandCharacteristic->setValue(stateMsg);
        commandCharacteristic->notify();
      }
    }

    else if (value == "CYCLE") {
      if (mainLightState) {
        brightnessIndex = (brightnessIndex + 1) % 3;
        analogWrite(mainLight, brightnessLvl[brightnessIndex]);
        Serial.print("Brightness cycled to: ");
        Serial.println(brightnessLvl[brightnessIndex]);
        std::string notifyMsg = "BRIGHTNESS:" + std::to_string(brightnessLvl[brightnessIndex]);
        commandCharacteristic->setValue(notifyMsg);
        commandCharacteristic->notify();
      } else {
        Serial.println("Main light is off — not cycling brightness");
      }
    }

    else if (value == "LEFT") {
      leftIsFlashing = !leftIsFlashing;
      Serial.println(leftIsFlashing ? "Left signal ON" : "Left signal OFF");
    }

    else if (value == "RIGHT") {
      rightIsFlashing = !rightIsFlashing;
      Serial.println(rightIsFlashing ? "Right signal ON" : "Right signal OFF");
    }

    else if (value.rfind("BRIGHTNESS:", 0) == 0) {
      int newBrightness = atoi(value.substr(11).c_str());
      newBrightness = constrain(newBrightness, 0, 255);  // Ensure valid range
      analogWrite(mainLight, newBrightness);
      mainLightState = (newBrightness > 0);  // Update state accordingly
      Serial.print("Main light brightness set to: ");
      Serial.println(newBrightness);
      std::string notifyMsg = "BRIGHTNESS:" + std::to_string(newBrightness);
      commandCharacteristic->setValue(notifyMsg);
      commandCharacteristic->notify();
      // Also notify light state
      std::string stateMsg = mainLightState ? "STATE:ON" : "STATE:OFF";
      commandCharacteristic->setValue(stateMsg);
      commandCharacteristic->notify();
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(mainLight, OUTPUT);
  pinMode(leftSignal, OUTPUT);
  pinMode(rightSignal, OUTPUT);
  analogWrite(mainLight, 0);
  digitalWrite(leftSignal, LOW);
  digitalWrite(rightSignal, LOW);

  // BLE Setup
  BLEDevice::init("HalosBeam-BLE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  commandCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  commandCharacteristic->setCallbacks(new BLECommandCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("🚀 BLE server ready and advertising.");
}

void loop() {
  // Blinking left signal
  if (leftIsFlashing) {
    if (millis() - leftLastFlashToggle >= flashInterval) {
      leftLastFlashToggle = millis();
      leftLightState = !leftLightState;
      digitalWrite(leftSignal, leftLightState);
    }
  } else {
    digitalWrite(leftSignal, LOW);
  }

  // Blinking right signal
  if (rightIsFlashing) {
    if (millis() - rightLastFlashToggle >= flashInterval) {
      rightLastFlashToggle = millis();
      rightLightState = !rightLightState;
      digitalWrite(rightSignal, rightLightState);
    }
  } else {
    digitalWrite(rightSignal, LOW);
  }

  // Optional: Debug connection status every 5s
  static unsigned long lastStatusCheck = 0;
  if (millis() - lastStatusCheck > 5000) {
    lastStatusCheck = millis();
    Serial.print("BLE Connected: ");
    Serial.println(bleConnected ? "Yes" : "No");
  }

  delay(50);
}