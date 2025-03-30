#include <M5StickCPlus.h> // Include the M5StickC Plus library

// Define pins
const int pirPin = 26; // PIR sensor SIG pin connected to GPIO 26
const int ledPin = 25; // LED connected to GPIO 25 (GPIO output)

void setup() {
  // Initialize the M5StickC Plus
  M5.begin();

  // Initialize the display
  M5.Lcd.setRotation(1); // Set screen orientation (0-3)
  M5.Lcd.fillScreen(BLACK); // Clear the screen
  M5.Lcd.setTextColor(WHITE); // Text color
  M5.Lcd.setTextSize(2); // Text size
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("PIR Motion Sensor");

  // Set pin modes
  pinMode(pirPin, INPUT);  // PIR sensor input
  pinMode(ledPin, OUTPUT); // LED output

  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("PIR Motion Sensor Test");
}

void loop() {
  // Read the PIR sensor value
  int motionDetected = digitalRead(pirPin);

  if (motionDetected == HIGH) {
    Serial.println("Motion detected!");
    digitalWrite(ledPin, HIGH); // Turn on LED
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.println("Motion detected!");
  } else {
    Serial.println("No motion detected.");
    digitalWrite(ledPin, LOW); // Turn off LED
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.println("No motion      "); // overwrite previous text
  }

  delay(200); // Short delay to avoid flooding
}
