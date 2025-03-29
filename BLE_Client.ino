#include "BLEDevice.h"
#include <M5StickCPlus.h>

//Default Temperature in Celsius
#define temperatureCelsius

// change the BLE Server name to connect to
#define bleServerName "AAA"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID bleServiceUUID("da1a59b9-1125-4bd4-b72b-d15dc8057c53");

//LED Service ID
static BLEUUID LEDStatusCharacteristicUUID("28ecdcde-5541-4db2-982c-91bde176da5d");

// BLE Characteristics
#ifdef temperatureCelsius
  // Temperature Celsius Characteristic
  static BLEUUID temperatureCharacteristicUUID("c10aa881-8c74-4146-bc26-4834ffb2ce5f");
#else
  // Temperature Fahrenheit Characteristic
  static BLEUUID temperatureCharacteristicUUID("44a8e759-5c4b-486d-917d-fec6454540e3");
#endif

// Battery Voltage Characteristic
static BLEUUID voltageCharacteristicUUID("01234567-0123-4567-89ab-0123456789ef");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
//Characteristicd that we want to read (what are the characteristics u want from the other party)
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* voltageCharacteristic;
static BLERemoteCharacteristic* LEDStatusCharacteristic; //LED status characteristic info comes here

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

//Variables to store temperature and voltage
char* temperatureStr;
char* voltageStr;

//Flags to check whether new temperature and voltage readings are available
boolean newTemperature = false;
boolean newVoltage = false;

bool LEDState;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bleServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.(get characteristics data and push into a local variable)
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(voltageCharacteristicUUID);
  LEDStatusCharacteristic = pRemoteService->getCharacteristic(LEDStatusCharacteristicUUID); // Tells program i want this characteristic data, regardless of notify or pulling of data.


  if (temperatureCharacteristic == nullptr || voltageCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  voltageCharacteristic->registerForNotify(voltageNotifyCallback);

  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
    else
      Serial.print(".");
  }
};
 
//When the BLE Server sends a new temperature reading with the notify property
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  temperatureStr = (char*)pData;
  newTemperature = true;
//  Serial.println("temperatureNotifyCallback");
}

//When the BLE Server sends a new voltage reading with the notify property
static void voltageNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store voltage value
  voltageStr = (char*)pData;
  newVoltage = true;
//  Serial.println("voltageNotifyCallback");
}

//function that prints the latest sensor readings in the OLED display
void printReadings(){
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.print("Temperature = ");
  M5.Lcd.print(temperatureStr);

  Serial.print("Temperature = ");
  Serial.print(temperatureStr);

  #ifdef temperatureCelsius
    //Temperature Celsius
    M5.Lcd.println(" C");
    Serial.print(" C");
  #else
    //Temperature Fahrenheit
    M5.Lcd.print(" F");
    Serial.print(" F");
  #endif

  //display voltage
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.print("Battery Voltage = ");
  M5.Lcd.print(voltageStr);
  M5.Lcd.println(" V");

  Serial.print(" - Battage Voltage = ");
  Serial.print(voltageStr); 
  Serial.println(" V");
}

void setup() {
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting BLE Client application...");

  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client", 0);

  //setup button
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);

  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server that we want to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.



  m5.update();
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("Connected to the BLE Server.");
      
      //Activate the Notify property of each Characteristic
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      voltageCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2903))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
      
    } else 
    {
      Serial.println("Failed to connect to the server; Restart device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }

  
  if (connected && m5.BtnA.wasPressed())
  {
    Serial.println("Home Button was pressed");
    std::string ledData = LEDStatusCharacteristic->readValue();
    size_t ledDataSize = ledData.length();
    char LEDBuffer[4];
    if (ledDataSize < sizeof(LEDBuffer))
    {
      memcpy(LEDBuffer, ledData.c_str(), ledDataSize);
      LEDBuffer[ledDataSize] = '\0';

      if(ledData[0] == '0')
      {
        LEDState = false;

      }
      else if(ledData[0]== '1')
      {
        LEDState = true;
      }
      else{
        Serial.println("Unexpected value for LED data.");
      }

      M5.Lcd.setCursor(0, 60, 2);
      M5.Lcd.print("LED Status = ");

      if (LEDState)
      {
        Serial.println("LED is Off");
        M5.Lcd.println("Off");

      }
      else
      {
        Serial.println("LED is On");
        M5.Lcd.println("On");
      }
      
    }
  }

  if (connected && m5.BtnB.wasPressed())
  {
    Serial.println("Button B  was pressed");
    
    LEDState = !LEDState;
    LEDStatusCharacteristic->writeValue(LEDState ? "1" : "0");
    Serial.println("LED state toggled on server");
    
  }
  

  //if new temperature readings are available, print in the OLED
  if (newTemperature && newVoltage){
    newTemperature = false;
    newVoltage = false;
    printReadings();
  }
  delay(1000); // Delay one second between loops.
}