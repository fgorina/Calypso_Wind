#ifndef BLE_MYCLIENT_H
#define BLE_MYCLIENT_H



#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CALYPSO_SERVICE "180D"
#define CALYPSO_DEVICE_SERVICE "180A"
#define ENVIRONMENTAL_SENSING_SERVICE "181A"

#define WIND_SPEED_CHARACTERISTIC "2A39" //"00002A39-0000-1000-8000-00805F9B34FB"
#define STATUS_CHARACTERISTIC "A001" // "0000A001-0000-1000-8000-00805F9B34FB"
#define DATA_RATE_CHARACTERISTIC "A002" // "0000A002-0000-1000-8000-00805F9B34FB"
#define SENSORS_CHARACTERISTIC "A003" // "0000A003-0000-1000-8000-00805F9B34FB"

#define APPARENT_WIND_SPEED_CHARACTERISTIC "2A72"
#define APPARENT_WIND_DIRECTION_CHARACTERISTIC "2A73"

#define MANUFACTURER_NAME_CHARACTERISTIC "2A29"
#define MODEL_NUMBER_CHARACTERISTIC "2A24"
#define SERIAL_NUMBER_CHARACTERISTIC "2A25"
#define HARDWARE_REVISION_CHARACTERISTIC "2A27"
#define FIRMWARE_REVISION_CHARACTERISTIC "2A26"
#define SOFTWARE_REVISION_CHARACTERISTIC "2A28"

typedef struct wind_data_t {

  int windSpeed = 0;
  int windDirection = 0;
  int batteryLevel = 0;
  int temperature = 0;
  int roll = 0;
  int pitch = 0;
  int compass = 0;

  uint16_t apparentWindSpeed = 0;
  uint16_t apparentWindDirection = 0;

} wind_data_t;

typedef struct device_info_t {
  String manufacturer = "";
  String modelNumber = "";
  String serialNumber = "";
  String hardwareRevision = "";
  String firmwareRevision = "";
  String softwareRevision = "";
}  device_info_t;

wind_data_t oldWindData;
wind_data_t windData;

device_info_t deviceInfo;

int scanTime = 5; //In seconds
BLEScan* pBLEScan;
bool windMeterFound = false;
bool windMeterConnected = false;
BLEAdvertisedDevice* windMeter;

BLEClient* bleClient;
BLERemoteService* pRemoteService;
BLERemoteService* environmentalService;
BLERemoteCharacteristic* windSpeedCharacteristic;
BLERemoteCharacteristic* apparentWindSpeedCharacteristic;
BLERemoteCharacteristic* apparentWindDirectionCharacteristic;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if(advertisedDevice.getName() == "ULTRASONIC"){         
          windMeter = new BLEAdvertisedDevice(advertisedDevice);
          windMeterFound = true;
          if(DEBUG_1){
            Serial.printf("Found Wind Meter: %s with %d db\n", advertisedDevice.toString().c_str());
          }
        }
      
    }
};

bool windDataEqual(wind_data_t w1, wind_data_t w2){

return w1.windSpeed == w2.windSpeed && w1.windDirection == w2.windDirection 
  && w1.batteryLevel == w2.batteryLevel
  && w1.temperature == w2.temperature
  && w1.roll == w2.roll
  && w1.pitch == w2.pitch
  && w1.compass == w2.compass
  && w1.apparentWindSpeed == w2.apparentWindSpeed
  && w1.apparentWindDirection == w2.apparentWindDirection;
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    windMeterConnected = true;
    if(DEBUG_1){
      Serial.println("Connected to Wind Meter");
    }
  }

  void onDisconnect(BLEClient* pclient) {
    windMeterConnected = false;
    clearBLELed();
    if(DEBUG_1){
      Serial.println("Disconnected to Wind Meter");
    }
  }
};
static void decodeWindData(uint8_t *data, size_t length){

  if(length >= 2){
    windData.windSpeed = data[1] * 256 + data[0];
  }

  if(length >= 4){
    windData.windDirection = data[3] * 256 + data[2];
  }

  if(length >= 5){
    windData.batteryLevel = data[4] * 10;
  }

  if(length >= 6){
    windData.temperature = data[5] -100;
  }

  if(length >= 7){
    windData.roll = data[6] - 90;
  }
  if(length >= 8){
    windData.pitch = data[7] - 90;
  }
  if(length >= 10){
    windData.compass = 360 - (data[9] * 256 + data[8]);
  }

}

static void printWindData(){
  Serial.println();
  Serial.print("Wind Speed: "); Serial.print(windData.windSpeed); Serial.println(" m/s * 100"); 
  Serial.print("Wind Direction: "); Serial.print(windData.windDirection); Serial.println("º"); 
  Serial.print("Battery Level: "); Serial.print(windData.batteryLevel); Serial.println("%"); 
  Serial.print("Temperature: "); Serial.print(windData.temperature); Serial.println("ºC"); 
  Serial.print("Roll: "); Serial.print(windData.roll); Serial.println("º"); 
  Serial.print("Pitch: "); Serial.print(windData.pitch); Serial.println("º"); 
  Serial.print("Compass: "); Serial.print(windData.compass); Serial.println("º"); 

  Serial.println("-------------------------------------------------------------");

}

static void printApparentData(){
  Serial.println();
  Serial.print("Apparent Wind Speed: "); Serial.print(windData.apparentWindSpeed); Serial.println(" m/s * 100"); 
  Serial.print("Apparent Wind Direction: "); Serial.print(windData.apparentWindDirection); Serial.println("º"); 
  Serial.println("-------------------------------------------------------------");

  
}

static void bleNotifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {


    decodeWindData(pData, length);
    sendData(windData.windSpeed, windData.windDirection, windData.batteryLevel);
    if (!windDataEqual(oldWindData, windData)){  
      if(DEBUG_1){
        printWindData();
      }
     oldWindData = windData;
    }
}


static void apparentWindSpeedCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    if (length >= 2){
      windData.apparentWindSpeed = pData[1] * 256 + pData[0];
    }
  
    if (!windDataEqual(oldWindData, windData)){
    if(DEBUG_1){
      printApparentData();
    }
     oldWindData = windData;
    }
}

static void apparentWindDirectionCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    if (length >= 2){
      windData.apparentWindDirection = pData[1] * 256 + pData[0];
    }
  
    if (!windDataEqual(oldWindData, windData)){
    if(DEBUG_1){
      printApparentData();
    }
     oldWindData = windData;
    }
}


void writeCommand(BLERemoteCharacteristic* characteristic,  uint8_t command){

    if(characteristic != nullptr && characteristic->canWrite()){
      if(DEBUG_1){
      Serial.print("Sending Command :"); Serial.println(command);
      }
        characteristic->writeValue(&command, 1);
    }
}

const char* getStringValue(BLERemoteService* service, const char* characteristic){
  BLERemoteCharacteristic* someChar = service->getCharacteristic(characteristic);
  if(someChar != nullptr){
    return someChar->readValue().c_str();

  }else{
    return "";
  }
}

uint8_t getUInt8(BLERemoteService* service, const char* characteristic){
  BLERemoteCharacteristic* someChar = service->getCharacteristic(characteristic);
  if(someChar != nullptr){
    return someChar->readUInt8();

  }else{
    return 0;
  }
}

void putUInt8(BLERemoteService* service, const char* characteristic, uint8_t value){
 BLERemoteCharacteristic* someChar = service->getCharacteristic(characteristic);
  if(someChar != nullptr && someChar->canWrite()){
    someChar->writeValue(value);
  }
}

void loadDeviceData(){
  BLERemoteService* deviceInfoService = bleClient->getService(CALYPSO_DEVICE_SERVICE);

  if (deviceInfoService == nullptr){
   
    Serial.println("No trobo el device info service");
  }else {
    Serial.println("Connected to device info sercice");
  }

  deviceInfo.manufacturer = getStringValue(deviceInfoService, MANUFACTURER_NAME_CHARACTERISTIC);
  deviceInfo.modelNumber = getStringValue(deviceInfoService, MODEL_NUMBER_CHARACTERISTIC);
  deviceInfo.serialNumber = getStringValue(deviceInfoService, SERIAL_NUMBER_CHARACTERISTIC);
  deviceInfo.hardwareRevision = getStringValue(deviceInfoService, HARDWARE_REVISION_CHARACTERISTIC);
  deviceInfo.firmwareRevision = getStringValue(deviceInfoService, FIRMWARE_REVISION_CHARACTERISTIC);
  deviceInfo.softwareRevision = getStringValue(deviceInfoService, SOFTWARE_REVISION_CHARACTERISTIC);

}

void printDeviceData(){
  Serial.println();
  Serial.print("Manufacturer : "); Serial.println(deviceInfo.manufacturer); 
  Serial.print("Model Number : "); Serial.println(deviceInfo.modelNumber); 
  Serial.print("Serial Number : "); Serial.println(deviceInfo.serialNumber); 
  Serial.print("Hardware Revision : "); Serial.println(deviceInfo.hardwareRevision); 
  Serial.print("Firmware Revision : "); Serial.println(deviceInfo.firmwareRevision); 
  Serial.print("Software Revision : "); Serial.println(deviceInfo.softwareRevision); 
  Serial.println("-------------------------------------------------------------");

}

bool connect_ble(){

    bleClient->connect(windMeter);
    if(DEBUG_1){
      Serial.println("Connected to Wind Meter");
    }

    loadDeviceData();
    if(DEBUG_1){
      printDeviceData();
    }

/*
    environmentalService = bleClient->getService(ENVIRONMENTAL_SENSING_SERVICE);
    if (environmentalService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(ENVIRONMENTAL_SENSING_SERVICE);
      return false;
    }

    apparentWindSpeedCharacteristic = environmentalService->getCharacteristic(APPARENT_WIND_SPEED_CHARACTERISTIC);
     if (apparentWindSpeedCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(APPARENT_WIND_SPEED_CHARACTERISTIC);
      bleClient->disconnect();
      return false;
    }
    apparentWindSpeedCharacteristic->registerForNotify(apparentWindSpeedCallback);

    apparentWindDirectionCharacteristic = environmentalService->getCharacteristic(APPARENT_WIND_DIRECTION_CHARACTERISTIC);
     if (apparentWindDirectionCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(APPARENT_WIND_DIRECTION_CHARACTERISTIC);
      bleClient->disconnect();
      return false;
    }
    apparentWindDirectionCharacteristic->registerForNotify(apparentWindDirectionCallback);

*/

    pRemoteService = bleClient->getService(CALYPSO_SERVICE);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(CALYPSO_SERVICE);
      return false;
    }
    Serial.println(" - Found our service");
   
   
    uint8_t  dataRate = getUInt8(pRemoteService, DATA_RATE_CHARACTERISTIC);
    Serial.print("DataRate: "); Serial.println(dataRate);

    uint8_t  sensorValue = getUInt8(pRemoteService, SENSORS_CHARACTERISTIC);
    Serial.print("Sensors value: "); Serial.println(sensorValue);


    if(sensorValue == 0 || true){
      Serial.println("Activating sensors");
      putUInt8(pRemoteService, SENSORS_CHARACTERISTIC, (uint8_t)1);
      sensorValue = getUInt8(pRemoteService, SENSORS_CHARACTERISTIC);
      Serial.print("Sensors value: "); Serial.println(sensorValue);
    }

    uint8_t  statusValue = getUInt8(pRemoteService, STATUS_CHARACTERISTIC);
    Serial.print("Status value: "); Serial.println(statusValue);
 
    windSpeedCharacteristic = pRemoteService->getCharacteristic(WIND_SPEED_CHARACTERISTIC);
    if (windSpeedCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(WIND_SPEED_CHARACTERISTIC);
      bleClient->disconnect();
      return false;
    }
    Serial.println(" - Found wind  characteristic");
    windSpeedCharacteristic->registerForNotify(bleNotifyCallback);

    setBLELed();

    return true;
}


void setup_ble() {

  if(windMeterConnected){
    return;
  }

  Serial.println("Starting BLE work!");

  BLEDevice::init("");
  
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.println("Scan done!");
  pBLEScan->clearResults(); 

  if (windMeterFound){
    Serial.print("I have found ");
    Serial.print(windMeter->getName().c_str());
    Serial.print(" at address ");
    Serial.println(windMeter->getAddress().toString().c_str());

    bleClient = BLEDevice::createClient();

    bool done = connect_ble();

  }
}

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif