#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cfloat> // 包含cfloat以获取FLT_MAX

// The remote service we wish to connect to.
static BLEUUID serviceUUID("90915b90-f8e9-45c1-a3eb-eba254edced9");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("de94a198-f148-449f-ba18-d7bb2c6ccf91");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Global variables for data collection
float maxValue = -FLT_MAX; // Initialize with minimum possible float value
float minValue = FLT_MAX;  // Initialize with maximum possible float value
float currentValue = 0;

// Function to process received data
void processData(String data) {
    int index = data.indexOf(':'); // 查找冒号的位置
    if (index != -1) {
        String numberString = data.substring(index + 2); // 从冒号后面的空格开始提取子字符串
        currentValue = numberString.toFloat(); // 将提取的子字符串转换为浮点数
    } else {
        currentValue = 0.0; // 如果格式不匹配，则设置为0
    }

    Serial.print("Received Data String: ");
    Serial.println(data);
    Serial.print("Converted Float Value: ");
    Serial.println(currentValue);

    if (currentValue > maxValue) {
        maxValue = currentValue;
    }
    if (currentValue < minValue) {
        minValue = currentValue;
    }

    // Print current, maximum, and minimum data
    Serial.print("Current Value: ");
    Serial.print(currentValue);
    Serial.print(" | Max Value: ");
    Serial.print(maxValue);
    Serial.print(" | Min Value: ");
    Serial.println(minValue);
}

static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();

    // Process the received data
    String receivedData = String((char*)pData);
    processData(receivedData);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("onDisconnect");
    }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server.
    pClient->connect(myDevice);  
    Serial.println(" - Connected to server");
    pClient->setMTU(517);

    // Obtain a reference to the service in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
        std::string value = pRemoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        } 
    }
}; 

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    if (doConnect == true) {
        if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
        } else {
            Serial.println("We have failed to connect to the server; there is nothing more we will do.");
        }
        doConnect = false;
    }

    if (connected) {
        // Client logic can be added here
    } else if (doScan) {
        BLEDevice::getScan()->start(0); 
    }

    delay(1000); // Delay a second between loops.
}