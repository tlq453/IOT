#include <BLEDevice.h>
#include <M5StickCPlus.h>

#define bleServerName "LightNode1"

// Service and characteristic UUIDs (must match LightNode.ino)
static BLEUUID serviceUUID("da1a59b9-1125-4bd4-b72b-d15dc8057c53");
static BLEUUID lightNameUUID("11111111-1111-1111-1111-111111111111");
static BLEUUID lightStateUUID("22222222-2222-2222-2222-222222222222");
static BLEUUID protocolUUID("33333333-3333-3333-3333-333333333333");

static boolean doConnect = false;
static boolean connected = false;
static BLEAddress *pServerAddress;
BLEClient* pClient = nullptr;

BLERemoteCharacteristic* stateChar = nullptr;
bool LEDState = false;

// === Notification Callback ===
void ledNotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify
) {
  if (length > 0) {
    char state = (char)pData[0];

    M5.Lcd.setCursor(0, 50, 2);
    M5.Lcd.fillRect(0, 50, 160, 20, BLACK); // clear previous text

    if (state == '1') {
      Serial.println("🔔 Notification: LED is ON");
      M5.Lcd.println("LED is ON");
    } else if (state == '0') {
      Serial.println("🔔 Notification: LED is OFF");
      M5.Lcd.println("LED is OFF");
    } else {
      Serial.println("⚠️ Notification: Unknown LED state");
    }
  }
}

// === Scan Callback ===
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) {
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      Serial.println("✅ Found LightNode. Connecting...");
      M5.Lcd.setCursor(0, 20, 2);
      M5.Lcd.println("Device Found");
    }
  }
};

// === Connect to Server ===
bool connectToServer(BLEAddress pAddress) {
  pClient = BLEDevice::createClient();
  pClient->connect(pAddress);
  Serial.println("🔗 Connected to server");

  BLERemoteService* service = pClient->getService(serviceUUID);
  if (!service) {
    Serial.println("❌ Could not find service");
    return false;
  }

  stateChar = service->getCharacteristic(lightStateUUID);
  if (!stateChar) {
    Serial.println("❌ Could not find lightState characteristic");
    return false;
  }

  if (stateChar->canNotify()) {
    stateChar->registerForNotify(ledNotifyCallback);
    Serial.println("📶 Registered for LED notifications");
  }

  return true;
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("BLE Client Ready");

  BLEDevice::init("");
  BLEScan* scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scanner->setActiveScan(true);
  scanner->start(30);
}

// === Loop ===
void loop() {
  if (doConnect) {
    if (connectToServer(*pServerAddress)) {
      connected = true;
      Serial.println("✅ BLE connection established.");
      //M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.println("BLE CONNECTED");
    } else {
      Serial.println("❌ Connection failed.");
      //M5.Lcd.setCursor(0, 40, 2);
      M5.Lcd.println("Connection Failed");
    }
    doConnect = false;
  }

  delay(500); // Light delay to allow async BLE processing
}
