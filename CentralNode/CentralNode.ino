#include <BLEDevice.h>
#include <M5StickCPlus.h>

// === BLE Server Names ===
#define bleServerName1 "LightNode1"  // With motion sensor
#define bleServerName2 "LightNode2"
#define bleServerName3 "LightNode3"

// === UUIDs (must match LightNode) ===
static BLEUUID serviceUUID("da1a59b9-1125-4bd4-b72b-d15dc8057c53");
static BLEUUID lightStateUUID("22222222-2222-2222-2222-222222222222");

// === Addresses & Characteristics ===
BLEAddress* LightNodeAddr1 = nullptr;
BLEAddress* LightNodeAddr2 = nullptr;
BLEAddress* LightNodeAddr3 = nullptr;

BLEClient* client1 = nullptr;
BLEClient* client2 = nullptr;
BLEClient* client3 = nullptr;

BLERemoteCharacteristic* StateChar1 = nullptr;
BLERemoteCharacteristic* StateChar2 = nullptr;
BLERemoteCharacteristic* StateChar3 = nullptr;

bool doConnect = false;
bool connected = false;

//== Function that executes the writing to other lightnode(currently 1 other lightnode)
void writeToAdjacentNode(char value) {
  Serial.printf("write to adjacentnode triggered");
  if (StateChar2 != nullptr && client2->isConnected()) {
    std::string val(1, value);  // convert char to string
    StateChar2->writeValue(val);
    Serial.printf("➡️ Sent '%c' to AdjacentNode\n", value);
  } else {
    Serial.println("⚠️ Cannot write: AdjacentNode not connected");
  }
}


// === Notification Callback for LightNode1 (motion) ===
void ledNotifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify) {
  if (len > 0) {
    char state = (char)pData[0];
    String status = (state == '1') ? "ON" : "OFF";

    // ==== LightNode1 triggers ====
    if (pChar == StateChar1) {
      Serial.printf("🔔 LightNode1 LED is %s\n", status.c_str());

      // Forward to AdjacentNode
      //writeToAdjacentNode(state);

      // Print LightNode1 status (line 1)
      M5.Lcd.setCursor(0, 50, 2);
      M5.Lcd.fillRect(0, 50, 160, 16, BLACK);
      M5.Lcd.printf("LightNode1: %s", status.c_str());
    }

    // ==== AdjacentNode responds ====
    else if (pChar == StateChar2) {
      Serial.printf("🔔 AdjacentNode LED is %s\n", status.c_str());

      // Print AdjacentNode status (line 2)
      M5.Lcd.setCursor(0, 70, 2);
      M5.Lcd.fillRect(0, 70, 160, 16, BLACK);
      M5.Lcd.printf("Adjacent: %s", status.c_str());
    }
  }
}

// === Scan Callback ===
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    std::string name = advertisedDevice.getName().c_str();

    if (name == bleServerName1 && LightNodeAddr1 == nullptr) {
      LightNodeAddr1 = new BLEAddress(advertisedDevice.getAddress());
      Serial.println("✅ Found LightNode1");
    }

    if (name == bleServerName2 && LightNodeAddr2 == nullptr) {
      LightNodeAddr2 = new BLEAddress(advertisedDevice.getAddress());
      Serial.println("✅ Found LightNode2");
    }

    if (name == bleServerName3 && LightNodeAddr3 == nullptr) {
      LightNodeAddr3 = new BLEAddress(advertisedDevice.getAddress());
      Serial.println("✅ Found LightNode3");
    }

    //if (LightNodeAddr1 && LightNodeAddr2 && LightNodeAddr3) {
    if (LightNodeAddr1 && LightNodeAddr2) {
      advertisedDevice.getScan()->stop();
      doConnect = true;
    }
    
  }
};

// === Connect Function ===
bool connectToServer(BLEAddress* addr, BLEClient*& client, BLERemoteCharacteristic*& characteristic) {
  client = BLEDevice::createClient();
  if (!client->connect(*addr)) return false;

  auto service = client->getService(serviceUUID);
  if (!service) return false;

  characteristic = service->getCharacteristic(lightStateUUID);
  return characteristic != nullptr;
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("CentralNode (3 Lights)");

  BLEDevice::init("");
  BLEScan* scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scanner->setActiveScan(true);
  scanner->start(30);  // Scan for up to 30s
}

// === Main Loop ===
void loop() {
  M5.update();

  if (doConnect) {
    // Connect to LightNode1 (the one with the motion sensor)
    if (connectToServer(LightNodeAddr1, client1, StateChar1)) {
      Serial.println("🔗 Connected to LightNode1");

      
      if (StateChar1->canNotify()) {
        StateChar1->registerForNotify(ledNotifyCallback);  // Important!
      }
    } else {
      Serial.println("❌ Failed to connect to LightNode1");
    }

    
    // Connect to LightNode2
    if (connectToServer(LightNodeAddr2, client2, StateChar2)) {
      Serial.println("🔗 Connected to LightNode2");
      if (StateChar2->canNotify()) {
        StateChar2->registerForNotify(ledNotifyCallback);
      }
    } else {
      Serial.println("❌ Failed to connect to LightNode2");
    }

    /*
    // Connect to LightNode3
    if (connectToServer(LightNodeAddr3, client3, StateChar3)) {
      Serial.println("🔗 Connected to LightNode3");
      if (StateChar3->canNotify()) {
        StateChar3->registerForNotify(ledNotifyCallback);
      }
    } else {
      Serial.println("❌ Failed to connect to LightNode3");
    }
    */

    connected = true;
    doConnect = false;
  }

  delay(500);
}

