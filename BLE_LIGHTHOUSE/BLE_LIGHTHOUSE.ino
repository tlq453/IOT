#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "35e65a71-b2d6-43f7-b3be-719ce5744884"
#define MOTION_STATE_CHAR_UUID     "36bd104c-a23f-4dcf-b808-a1bc4516314e"
#define LED_STATE_CHAR_UUID     "c033e6dc-1355-4935-abb1-6e46b1a84c36"
#define M5_LED             10  // Built-in RED LED on GPIO 10

BLEServer *pServer;
BLECharacteristic *pMotionChar;
BLECharacteristic *pLedChar;

// Define States
bool ledOn = false;
bool motionDetected = false;

// Define Pins
const int pirPin = 26;
const int ledPin = 25;

class LedCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            uint8_t ledState = value[0];
            ledOn = (ledState == 1);
            digitalWrite(ledPin, ledOn ? HIGH : LOW);  // LOW turns ON the LED
            Serial.printf("LED set to: %s\n", ledOn ? "ON" : "OFF");
            
            // Update the characteristic value and notify
            pLedChar->setValue(&ledState, 1);
            pLedChar->notify();  // Added this line to notify on change
        }
    }
};

void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("Bluetooth Connection Test");

  pinMode(pirPin, INPUT);

  pinMode(ledPin, OUTPUT);  // Set LED pin as OUTPUT
  digitalWrite(ledPin, LOW); // Initialize OFF

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("BLE LED/Motion Server");


  BLEDevice::init("M5StickCPlus-LED-EDRIC");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Motion State Characteristic
  pMotionChar = pService->createCharacteristic(
    MOTION_STATE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pMotionChar->addDescriptor(new BLE2902());
  uint8_t initialMotionState = 0;
  pMotionChar->setValue(&initialMotionState, 1);

  // LED State Characteristic - Added NOTIFY property
  pLedChar = pService->createCharacteristic(
      LED_STATE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY  // Added this line
  );
  pLedChar->addDescriptor(new BLE2902());
  pLedChar->setCallbacks(new LedCallback());
  uint8_t initialLedState = 1;
  pLedChar->setValue(&initialLedState, 1);
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("BLE Server Started");
}

void loop() {
    M5.update();
    static unsigned long lastNotificationTime = 0;
    const unsigned long notificationInterval = 1000; // 1 seconds
    
    // Read motion sensor
    bool currentMotion = digitalRead(pirPin);
    
    // Update motion state if changed or time interval elapsed
    if (currentMotion != motionDetected && (millis() - lastNotificationTime >= notificationInterval)) {
        motionDetected = currentMotion;
        uint8_t motionState = motionDetected ? 1 : 0;
        pMotionChar->setValue(&motionState, 1);
        pMotionChar->notify();
        
        Serial.printf("Motion %s\n", motionDetected ? "DETECTED" : "CLEAR");
        lastNotificationTime = millis();
    }

  if (M5.BtnA.wasPressed()) {
    ledOn = !ledOn;
    digitalWrite(ledPin, ledOn ? HIGH : LOW);  // Toggle LED
    Serial.println("Button Pressed!"); // Log to Serial Monitor
    
    uint8_t ledState = ledOn ? 1 : 0;
    pLedChar->setValue(&ledState, 1);
    pLedChar->notify();  // Added this line to notify on button press
    
    M5.Lcd.printf("LED: %s\n", ledOn ? "ON" : "OFF");
  }
  delay(100);
}