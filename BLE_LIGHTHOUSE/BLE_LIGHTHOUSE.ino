#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "35e65a71-b2d6-43f7-b3be-719ce5744884"
#define STATE_CHAR_UUID     "c033e6dc-1355-4935-abb1-6e46b1a84c36"
#define M5_LED             10  // Built-in RED LED on GPIO 10

BLEServer *pServer;
BLECharacteristic *pStateChar;
bool ledOn = false;

void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("Bluetooth Connection Test");

  M5.Lcd.println("BLE LED State Server");
  pinMode(M5_LED, OUTPUT);  // Set LED pin as OUTPUT
  digitalWrite(M5_LED, HIGH); // Initialize OFF

  BLEDevice::init("M5StickCPlus-LED-EDRIC");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pStateChar = pService->createCharacteristic(
    STATE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pStateChar->addDescriptor(new BLE2902());

  uint8_t initialState = 0;
  pStateChar->setValue(&initialState, 1);
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    ledOn = !ledOn;
    digitalWrite(M5_LED, ledOn ? LOW : HIGH);  // Toggle LED
    Serial.println("Button Pressed!"); // Log to Serial Monitor
    
    uint8_t state = ledOn ? 1 : 0;
    pStateChar->setValue(&state, 1);
    pStateChar->notify();  // Notify Raspberry Pi
    
    M5.Lcd.printf("LED: %s\n", ledOn ? "ON" : "OFF");
  }
  delay(100);
}