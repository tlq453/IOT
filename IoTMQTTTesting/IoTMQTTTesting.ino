#include "M5StickCPlus.h"
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <sys/time.h> 

#define M5_LED             10  // Built-in RED LED on GPIO 10

// Define pins
const int pirPin = 26; // PIR sensor SIG pin connected to GPIO 26
const int ledPin = 25; // LED connected to GPIO 33

// WiFi and MQTT settings
const char* ssid = R"(Sim’s iPhone)";
const char* password = "a1234567";
const char* mqtt_server = "172.20.10.3";

// WiFi and MQTT Client setup
WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, mqtt_server, 1883);  // Direct MQTT Client connection

// MQTT Topics
Adafruit_MQTT_Publish PIR_A = Adafruit_MQTT_Publish(&mqtt, "building/rooms/A/PIR/A", 1); // QoS 1 publication
Adafruit_MQTT_Subscribe PIR_A_SUB = Adafruit_MQTT_Subscribe(&mqtt, "building/rooms/A/PIR/A", 1); // QoS 1 subscription
Adafruit_MQTT_Publish LED_A = Adafruit_MQTT_Publish(&mqtt, "building/rooms/A/LED/A", 1); // QoS 1 publication

// Timer variables
unsigned long lastMotionTime = 0;
bool cooldownMotion = false;
// unsigned long timer_interval = 240000;
// unsigned long led_interval = 300000;
unsigned long timer_interval = 4000;
unsigned long led_interval = 5000;

// ledState isSent
bool isSent = false;

int currentState = 1;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

unsigned long startTime = 0;
unsigned long endTime = 0;

// void flashLED();
void setupWifi();
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    setupWifi();

    mqtt.subscribe(&PIR_A_SUB);
    
    // Setup LED
    pinMode(pirPin, INPUT);
    pinMode(ledPin, OUTPUT);
    pinMode(M5_LED, OUTPUT);
}



void loop() {
    if (!mqtt.connected()) {
        reConnect();
    }
    mqtt.processPackets(10);  // Process incoming MQTT messages with a timeout of 10 millisecond
    mqtt.ping();

    M5.update();
    
    // Button press to Simulate Motion Detected
    if (M5.BtnA.wasPressed()) {
        Serial.println("Motion detected!"); // Log to Serial Monitor

        snprintf(msg, MSG_BUFFER_SIZE, "1");
        startTime = millis();
        PIR_A.publish(msg);

        M5.Lcd.println("Motion Detected");
    }

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(10))) {
      if (subscription == &PIR_A_SUB) {
        endTime = millis();
        unsigned long elapsedTime = endTime - startTime;

        Serial.print("Elapsed Time: ");
        Serial.println(elapsedTime);

        char* message = (char *) PIR_A_SUB.lastread;
        if (strcmp(message, "1") == 0) {
          digitalWrite(M5_LED, currentState ? 0 : 1);
          snprintf(msg, MSG_BUFFER_SIZE, "1");

          LED_A.publish(msg);
          M5.Lcd.println("Sent ON");
        }
      }
    }
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
            if (!mqtt.subscribe(&PIR_A_SUB)) {
                M5.Lcd.println("❌ Sub failed");
            } else {
                M5.Lcd.println("✅ Sub success");
            }
        } else {
            M5.Lcd.print("MQTT connect failed: ");
            M5.Lcd.println(mqtt.connectErrorString(ret));
            delay(5000);
        }
    }
}
