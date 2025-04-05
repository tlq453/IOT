#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "35e65a71-b2d6-43f7-b3be-719ce5744884"
#define LED_STATE_CHAR_UUID "c033e6dc-1355-4935-abb1-6e46b1a84c36"
#define M5_LED             10  // Built-in RED LED on GPIO 10

BLEServer *pServer;
BLECharacteristic *pLedChar;

// Define States
bool ledOn = false;

// Define LED Pin
const int ledPin = 25;

class LedCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t ledState = value[0];
            ledOn = (ledState == 1);
            digitalWrite(ledPin, ledOn ? HIGH : LOW);
            Serial.printf("LED set to: %s\n", ledOn ? "ON" : "OFF");
            
            // Update the characteristic value
            pLedChar->setValue(&ledState, 1);
        }
    }
};

void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("Bluetooth LED Control Server");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Initialize OFF

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("BLE LED Server");

  BLEDevice::init("M5StickCPlus-LED-EDRIC2");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // LED State Characteristic
  pLedChar = pService->createCharacteristic(
      LED_STATE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
  );
  pLedChar->addDescriptor(new BLE2902());
  pLedChar->setCallbacks(new LedCallback());
  uint8_t initialLedState = 0;
  pLedChar->setValue(&initialLedState, 1);
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("BLE Server Started");
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    ledOn = !ledOn;
    digitalWrite(ledPin, ledOn ? HIGH : LOW);
    Serial.println("Button Pressed!");
    
    uint8_t ledState = ledOn ? 1 : 0;
    pLedChar->setValue(&ledState, 1);
    
    M5.Lcd.printf("LED: %s\n", ledOn ? "ON" : "OFF");
  }
  delay(100);
}