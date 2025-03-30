#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>
#include <M5StickCPlus.h>

//#include <Wire.h>

//Default Temperature in Celsius
#define temperatureCelsius

//change to unique BLE server name
#define bleServerName "AAA"

float tempC = 25.0;
float tempF;
float vBatt = 5.0;  // initial value
bool LedState;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;   // update refresh every 15sec

bool deviceConnected = false;


// LED setup
const int LED_PIN = 10;  


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "da1a59b9-1125-4bd4-b72b-d15dc8057c53"


// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
    BLECharacteristic imuTemperatureCelsiusCharacteristics("c10aa881-8c74-4146-bc26-4834ffb2ce5f", BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor imuTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#else
    BLECharacteristic imuTemperatureFahrenheitCharacteristics("44a8e759-5c4b-486d-917d-fec6454540e3", BLECharacteristic::PROPERTY_NOTIFY);
    BLEDescriptor imuTemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2902));
#endif

// Battery Voltage Characteristic and Descriptor
BLECharacteristic axpVoltageCharacteristics("01234567-0123-4567-89ab-0123456789ef", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpVoltageDescriptor(BLEUUID((uint16_t)0x2903));

// LED characteristic for users to access this service
BLECharacteristic ledCharacteristic("28ecdcde-5541-4db2-982c-91bde176da5d", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor ledDescriptor(BLEUUID((uint16_t)0x2901));


//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Connected to Client");
        Serial.println("MyServerCallbacks:;:Connected...");
    };
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        BLEDevice::startAdvertising();
        M5.Lcd.printf("Advertising restarted");
        M5.Lcd.printf("MyServerCallbacks::Disconnected...");
    }
};

class LEDCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
        if (value[0] == '0') {
            LedState = true;
            digitalWrite(LED_PIN, LOW); // Turn on LED
            Serial.println("LED turned on by client");
        } else if (value[0] == '1') {
            LedState = false;
            digitalWrite(LED_PIN, HIGH); // Turn off LED
            Serial.println("LED turned off by client");
        } else {
            Serial.println("Unexpected value for LED data.");
        }
        }
    }
};


void setup() {
    // Start serial communication 
    Serial.begin(115200);

    // put your setup code here, to run once:
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0, 2);
    M5.Lcd.printf("BLE Server", 0);

    //Config the Button m5_button_home
    pinMode(M5_BUTTON_HOME, INPUT);

    //setup LED
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Turn off LED initially


    // Create the BLE Device
    BLEDevice::init(bleServerName);
    Serial.println("BLE Device initialized\n");

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    Serial.println("BLE Server created\n");

    // Create the BLE Service
    BLEService *bleService = pServer->createService(SERVICE_UUID);
    Serial.println("BLE Service created\n");

    // Create BLE Characteristics and Create a BLE Descriptor
    // Temperature
    #ifdef temperatureCelsius
        bleService->addCharacteristic(&imuTemperatureCelsiusCharacteristics);
        imuTemperatureCelsiusDescriptor.setValue("IMU Temperature(C)");
        imuTemperatureCelsiusCharacteristics.addDescriptor(&imuTemperatureCelsiusDescriptor);
    #else
        bleService->addCharacteristic(&imuTemperatureFahrenheitCharacteristics);
        imuTemperatureFahrenheitDescriptor.setValue("IMU Temperature(F)");
        imuTemperatureFahrenheitCharacteristics.addDescriptor(&imuTemperatureFahrenheitDescriptor);
    #endif  

    // Battery
    bleService->addCharacteristic(&axpVoltageCharacteristics);
    axpVoltageDescriptor.setValue("AXP Battery(V)");
    axpVoltageCharacteristics.addDescriptor(&axpVoltageDescriptor); 

    //Add LED charcteristic to the service
    bleService->addCharacteristic(&ledCharacteristic);
    ledDescriptor.setValue("LED State");
    ledCharacteristic.addDescriptor(&ledDescriptor);
    ledCharacteristic.setCallbacks(new LEDCharacteristicCallbacks()); // Register the callback

        
    // Start the service
    bleService->start();
    Serial.println("BLE Service started\n");

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
}

void loop() {

    Serial.println("Server is up");
    delay(2000);
    m5.update();
    if (deviceConnected) {
        //Do some code when server is connected
        Serial.println("Device is connected to the server");




  }
}
