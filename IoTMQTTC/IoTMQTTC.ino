#include "M5StickCPlus.h"
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// Define pins
const int pirPin = 26; // PIR sensor SIG pin connected to GPIO 26
const int ledPin = 10; // LED connected to GPIO 33

// WiFi and MQTT settings
const char* ssid = "Sim's iPhone";
const char* password = "a1234567";
const char* mqtt_server = "172.20.10.2";

// WiFi and MQTT Client setup
WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, mqtt_server, 1883);  // Direct MQTT Client connection

// MQTT Topics
Adafruit_MQTT_Subscribe PIR_A = Adafruit_MQTT_Subscribe(&mqtt, "building/rooms/A/PIR/A", 1); // QoS 1 subscription
Adafruit_MQTT_Publish LED_C = Adafruit_MQTT_Publish(&mqtt, "building/rooms/A/LED/C", 1); // QoS 1 publication

// Timer variables
unsigned long lastMotionTime = 0;
unsigned long led_interval = 300000;

// ledState isSent
bool isSent = false;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setupWifi();
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    setupWifi();
    
    // Subscribe to the "building/rooms/A/PIR/A" topic with QoS 1
    mqtt.subscribe(&PIR_A);  // Subscribe to "building/rooms/A/PIR/A"

    // Setup LED
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, 1);
}

void loop() {
    if (!mqtt.connected()) {
        reConnect();
    }
    mqtt.processPackets(10);  // Process incoming MQTT messages with a timeout of 10 millisecond
    mqtt.ping();

    // M5.update();

    unsigned long currentMillis = millis();
    if (currentMillis - lastMotionTime >= led_interval) {
      if (isSent == false) {
        digitalWrite(ledPin, 1); // Turn OFF LED
        snprintf(msg, MSG_BUFFER_SIZE, "0");
        LED_C.publish(msg);
        isSent = true;
      }
    }
    
    // Check for incoming messages on "toggle_LED/A"
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(10))) {
      if (subscription == &PIR_A) {
        char* message = (char *) PIR_A.lastread;
        if (strcmp(message, "ON") == 0) {
          lastMotionTime = currentMillis;
          digitalWrite(ledPin, 0);
          snprintf(msg, MSG_BUFFER_SIZE, "1");
          LED_C.publish(msg);
          M5.Lcd.println("Sent ON");
          isSent = false;
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
        
        int8_t ret = mqtt.connect("ledC", "password3");

        if (ret == 0) {
            M5.Lcd.println("Connected");
            mqtt.subscribe(&PIR_A);
        } else {
            M5.Lcd.print("MQTT connect failed: ");
            M5.Lcd.println(mqtt.connectErrorString(ret));
            delay(5000);
        }
    }
}
