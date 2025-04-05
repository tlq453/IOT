#include "M5StickCPlus.h"
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// Define pins
const int pirPin = 26; // PIR sensor SIG pin connected to GPIO 26
const int ledPin = 25; // LED connected to GPIO 33

// WiFi and MQTT settings
const char* ssid = R"(Sim’s iPhone)";
const char* password = "a1234567";
const char* mqtt_server = "172.20.10.2";

// WiFi and MQTT Client setup
WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, mqtt_server, 1883);  // Direct MQTT Client connection

// MQTT Topics
Adafruit_MQTT_Publish PIR_A = Adafruit_MQTT_Publish(&mqtt, "building/rooms/A/PIR/A", 1); // QoS 1 publication
Adafruit_MQTT_Publish LED_A = Adafruit_MQTT_Publish(&mqtt, "building/rooms/A/LED/A", 1); // QoS 1 publication

// Timer variables
unsigned long lastMotionTime = 0;
bool cooldownMotion = false;
unsigned long timer_interval = 240000;
unsigned long led_interval = 300000;

// ledState isSent
bool isSent = false;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

// void flashLED();
void setupWifi();
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    setupWifi();
    
    // Subscribe to the "toggle_LED/A" topic with QoS 1
    // mqtt.subscribe(&toggleLED_A);  // Subscribe to "toggle_LED/A"

    // Setup LED
    pinMode(pirPin, INPUT);
    pinMode(ledPin, OUTPUT);
}

// void flashLED() {
//     M5.Lcd.fillRect(0, 60, 240, 20, BLACK);
//     M5.Lcd.setCursor(0, 60, 2);
//     M5.Lcd.print("LED ON");

//     digitalWrite(ledPin, 0);  // LED ON
//     delay(500);
//     digitalWrite(ledPin, 1);  // LED OFF

//     M5.Lcd.fillRect(0, 60, 240, 20, BLACK);
//     M5.Lcd.setCursor(0, 60, 2);
//     M5.Lcd.print("LED OFF");
// }

void loop() {
    if (!mqtt.connected()) {
        reConnect();
    }
    mqtt.processPackets(10);  // Process incoming MQTT messages with a timeout of 10 millisecond
    mqtt.ping();

    // M5.update();
    
    // // Button press to toggle LED
    // if (M5.BtnA.wasPressed()) {
    //     if ((millis() - buttonLastTime) > debounceTime) {
    //         snprintf(msg, MSG_BUFFER_SIZE, "ON");
    //         toggleLED_B.publish(msg);  // Publish message to "toggle_LED/B"
    //         buttonLastTime = millis();
    //         M5.Lcd.println("Sent ON");
    //     }
    // }

    // Read the PIR sensor value
    int motionDetected = digitalRead(pirPin);
    unsigned long currentMillis = millis();

    if (cooldownMotion) {
        // Check if cooldown period is over
        if (currentMillis - lastMotionTime >= timer_interval) {
            cooldownMotion = false;
            // M5.Lcd.println("Cooldown ended");
        }
    }
    if (currentMillis - lastMotionTime >= led_interval) {
      if (isSent = false) {
          digitalWrite(ledPin, 0); // Turn OFF LED
          snprintf(msg, MSG_BUFFER_SIZE, "0");
          LED_A.publish(msg);
          isSent = true;
      }
    }

    // If motion is detected, blink the LED
    if (!cooldownMotion) {
      if (motionDetected == HIGH) {
        Serial.println("Motion detected!"); // Log to Serial Monitor
        lastMotionTime = currentMillis;
        cooldownMotion = true;
        snprintf(msg, MSG_BUFFER_SIZE, "1");
        PIR_A.publish(msg);
        digitalWrite(ledPin, 1); // Turn on the LED
        snprintf(msg, MSG_BUFFER_SIZE, "1");
        LED_A.publish(msg);
        M5.Lcd.println("Sent ON");
        isSent = false;
      } else {
        Serial.println("No motion detected."); // Log to Serial Monitor
      }
    }

    delay(200);
    
    // // Check for incoming messages on "toggle_LED/A"
    // Adafruit_MQTT_Subscribe *subscription;
    // while ((subscription = mqtt.readSubscription(10))) {
    //   if (subscription == &toggleLED_A) {
    //     char* message = (char *) toggleLED_A.lastread;
    //     if (strcmp(message, "ON") == 0) {
    //       flashLED();
    //       snprintf(msg, MSG_BUFFER_SIZE, "A is ON");
    //       toggleLED_B.publish(msg);
    //     }
    //   }
    // }
}

void setupWifi() {
    delay(10);
    M5.Lcd.printf("Connecting to %s", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.printf("\nConnected\n");
}

void reConnect() {
    while (!mqtt.connected()) {
        M5.Lcd.print("Attempting MQTT connection...");
        
        int8_t ret = mqtt.connect("ledA", "password1");

        if (ret == 0) {
            M5.Lcd.println("Connected");
            //mqtt.subscribe(&toggleLED_A);
        } else {
            M5.Lcd.print("MQTT connect failed: ");
            M5.Lcd.println(mqtt.connectErrorString(ret));
            delay(5000);
        }
    }
}
