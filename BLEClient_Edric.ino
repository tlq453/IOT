#include <ArduinoBLE.h>

// --- Pin Definitions ---
#define MOTION_PIN 13    // Digital pin for motion sensor input
#define LED_PIN1 2       // LED output pins
#define LED_PIN2 3
#define LED_PIN3 4
#define LED_PIN4 5

// --- BLE Service and Characteristic UUIDs ---
#define SMARTLIGHT_SERVICE_UUID         "12345678-1234-5678-1234-56789abcdef0"
#define MOTION_CHARACTERISTIC_UUID      "12345678-1234-5678-1234-56789abcdef1"
#define LED_CONTROL_CHARACTERISTIC_UUID "12345678-1234-5678-1234-56789abcdef2"

// Create the BLE Service and Characteristics
BLEService smartLightService(SMARTLIGHT_SERVICE_UUID);
BLEByteCharacteristic motionChar(MOTION_CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEByteCharacteristic ledControlChar(LED_CONTROL_CHARACTERISTIC_UUID, BLEWrite);

byte motionState = 0;  // 0: no motion, 1: motion detected
byte ledState = 0;     // 0: off, 1: standby (2 LEDs), 2: full (4 LEDs)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize pin modes
  pinMode(MOTION_PIN, INPUT);
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
  pinMode(LED_PIN4, OUTPUT);

  // Start with all LEDs off
  updateLEDs(0);

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }
  
  BLE.setLocalName("SmartLightPeripheral");
  BLE.setAdvertisedService(smartLightService);

  // Add characteristics to the service
  smartLightService.addCharacteristic(motionChar);
  smartLightService.addCharacteristic(ledControlChar);
  BLE.addService(smartLightService);

  // Set initial characteristic values
  motionChar.writeValue(motionState);
  ledControlChar.writeValue(ledState);

  // Start advertising the BLE service
  BLE.advertise();
  Serial.println("SmartLightPeripheral is now advertising...");
}

void loop() {
  // Wait for a central device to connect
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    
    while (central.connected()) {
      // Read the motion sensor and update the motion characteristic
      int sensorValue = digitalRead(MOTION_PIN);
      motionState = (sensorValue == HIGH) ? 1 : 0;
      motionChar.writeValue(motionState);

      // Check for incoming LED control commands from the central
      if (ledControlChar.written()) {
        ledState = ledControlChar.value();
        Serial.print("Received LED command: ");
        Serial.println(ledState);
        updateLEDs(ledState);
      }
      delay(100);
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

// Function to update LED outputs based on the received command
void updateLEDs(byte state) {
  // Turn off all LEDs first
  digitalWrite(LED_PIN1, LOW);
  digitalWrite(LED_PIN2, LOW);
  digitalWrite(LED_PIN3, LOW);
  digitalWrite(LED_PIN4, LOW);
  
  if (state == 1) {  // Standby mode: Turn on 2 LEDs
    digitalWrite(LED_PIN1, HIGH);
    digitalWrite(LED_PIN2, HIGH);
  } else if (state == 2) {  // Full mode: Turn on all 4 LEDs
    digitalWrite(LED_PIN1, HIGH);
    digitalWrite(LED_PIN2, HIGH);
    digitalWrite(LED_PIN3, HIGH);
    digitalWrite(LED_PIN4, HIGH);
  }
  // For state 0, leave all LEDs off.
}
