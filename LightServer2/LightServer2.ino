#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEServer.h>

// ====== Pin and BLE Definitions ======
#define LED_PIN 25
#define PIR_PIN 26
#define BLE_SERVER_NAME "LightNode2"

#define SERVICE_UUID "da1a59b9-1125-4bd4-b72b-d15dc8057c53"
#define LIGHT_NAME_UUID    "11111111-1111-1111-1111-111111111111"
#define LIGHT_STATE_UUID   "22222222-2222-2222-2222-222222222222"
#define PROTOCOL_UUID      "33333333-3333-3333-3333-333333333333"

// ====== BLE Characteristics ======
BLECharacteristic lightNameChar(LIGHT_NAME_UUID, BLECharacteristic::PROPERTY_READ);
BLECharacteristic lightStateChar(LIGHT_STATE_UUID,BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_WRITE |BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic protocolChar(PROTOCOL_UUID, BLECharacteristic::PROPERTY_READ);

BLEServer* pServer = nullptr;
bool deviceConnected = false;

// ====== Timing Variables ======
unsigned long lastMotionCheck = 0;
unsigned long motionCheckInterval = 1000; // Default: 1 second
unsigned long ledOffTime = 0;

// ====== LED State Logic ======
void turnOnLED() {
  digitalWrite(LED_PIN, HIGH); // Turn ON (assuming active HIGH)
  lightStateChar.setValue("1");
  if (deviceConnected) lightStateChar.notify();
  Serial.println("LED turned ON");
  ledOffTime = millis() + 5 * 1000; // Schedule auto-off in 5 minutes
  motionCheckInterval = 4 * 1000;   // Slow down motion checks
  M5.Lcd.setCursor(0, 50, 2);
  M5.Lcd.fillRect(0, 50, 160, 20, BLACK);
  M5.Lcd.println("LED IS turned ON");
}

void turnOffLED() {
  digitalWrite(LED_PIN, LOW); // Turn OFF
  lightStateChar.setValue("0");
  if (deviceConnected) lightStateChar.notify();   // ✅ CRUCIAL LINE
  Serial.println("LED turned OFF");
  Serial.println("[DEBUG] Notifying CentralNode: LED OFF");
  motionCheckInterval = 1000; // Resume 1-second motion polling
  ledOffTime = 0;
  M5.Lcd.setCursor(0, 50, 2);
  M5.Lcd.fillRect(0, 50, 160, 20, BLACK);
  M5.Lcd.println("LED IS turned off");
}

bool motionDetected() {
  return digitalRead(PIR_PIN) == HIGH;
}

// ====== BLE Callbacks ======
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    M5.Lcd.println("Device Connected");
    Serial.println("BLE connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE disconnected - restarting advertising");
    pServer->getAdvertising()->start();
  }
};

class LEDCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value == "1") {
      digitalWrite(25, HIGH);
      Serial.println("LED turned ON");

      // Display on LCD
      M5.Lcd.fillRect(0, 50, 160, 20, BLACK);
      M5.Lcd.setCursor(0, 50, 2);
      M5.Lcd.printf("LED is ON");

    } else {
      digitalWrite(25, LOW);
      Serial.println("LED turned OFF");

      // Display on LCD
      M5.Lcd.fillRect(0, 50, 160, 20, BLACK);
      M5.Lcd.setCursor(0, 50, 2);
      M5.Lcd.printf("LED is OFF");
    }
  }
};

// ====== BLE Setup ======
void setupBLE() {
  BLEDevice::init(BLE_SERVER_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *service = pServer->createService(SERVICE_UUID);

  lightNameChar.setValue("LightNode 2");
  protocolChar.setValue("BLE");

  service->addCharacteristic(&lightNameChar);
  service->addCharacteristic(&lightStateChar);
  service->addCharacteristic(&protocolChar);
  lightStateChar.setCallbacks(new LEDCharacteristicCallbacks());

  service->start();

  BLEAdvertising *advertising = pServer->getAdvertising();  // <- get from your server
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();

  Serial.println("BLE advertising started");
}

// ====== Arduino Setup ======
void setup() {
  M5.begin();
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with LED OFF

  pinMode(PIR_PIN, INPUT);    // Motion sensor
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("Light Node 2");

  setupBLE();
}

// ====== Arduino Main Loop ======
void loop() {
  unsigned long now = millis();
  

  if (deviceConnected) {
    Serial.println("in device connected loop");
    
    // Auto turn-off check
    if (ledOffTime > 0 && now >= ledOffTime) {
      Serial.println("Auto-off: LED turned OFF after 5 minutes");
      turnOffLED();
    }
  }

  delay(1000);
}
