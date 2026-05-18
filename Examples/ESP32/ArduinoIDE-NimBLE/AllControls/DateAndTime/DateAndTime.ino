// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/DateAndTime/DateAndTime.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "00000090-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "10000090-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TIME32_UUID       "10000091-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_DATE32_UUID       "10000092-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_DATE64_UUID       "10000093-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_DATETIME32_UUID   "10000094-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_DATETIME64_UUID   "10000095-74ee-43ce-86b2-0dde20dcefd6"
// Default UUID mask for the BLE Manager app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by BLE Manager to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID "2000face-74ee-43ce-86b2-0dde20dcefd6"

// Custom server callback class to handle connection events
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    Serial.println("Device connected.");
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    Serial.println("Device disconnected. Restarting advertising...");
    // Restart advertising when a device disconnects
    NimBLEDevice::startAdvertising();
  }
};

class DateTimeCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    const size_t timeSize = sizeof(time_t);
    time_t receivedTime = 0;

    for (size_t i = 0; i < timeSize; i++) {
      if (i < dataLength) {
        receivedTime |= ((uint64_t)(data[i] & 0xFF) << (8 * i));
      }
    }

    char buffer[64];
    struct tm *timeinfo = localtime(&receivedTime);
    if (timeinfo) {
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
      Serial.print("Received time: ");
      Serial.println(buffer);
    } else {
      Serial.println("Invalid time value (out of range)!");
    }
  }
};

void setup() {
  Serial.begin(115200);

  struct tm dtm = {};
  dtm.tm_year = 2025 - 1900;
  dtm.tm_mon = 0;
  dtm.tm_mday = 1;
  dtm.tm_hour = 11;
  dtm.tm_min = 59;
  dtm.tm_sec = 0;

  struct tm dt = {};
  dt.tm_year = 2025 - 1900;
  dt.tm_mon = 0;
  dt.tm_mday = 1;
  dt.tm_hour = 0;
  dt.tm_min = 0;
  dt.tm_sec = 0;

  // Initialize BLE device with a name
  NimBLEDevice::init("BLE Device");

  // Create a BLE server and set its callback class
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create a BLE service. Unlike Bluedroid, NimBLE allocates handles
  // dynamically — no numHandles argument required.
  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE characteristic for service name
  // The value of this characteristic will be displayed as the service name.
  NimBLECharacteristic *pCharacteristicServiceName = pService->createCharacteristic(
    CHARACTERISTIC_SERVICE_NAME_UUID,
    NIMBLE_PROPERTY::READ);

  {
    const char *json = R"({"type":"serviceName", "order":1})";
    NimBLEDescriptor *desc = pCharacteristicServiceName->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicServiceName->setValue("Date And Time");

  //// CONTROLS ////

  // time: editable control for time (uint16_t) characteristics
  // The value represents the number of seconds since midnight.
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicTime = pService->createCharacteristic(
    CHARACTERISTIC_TIME32_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicTime->setCallbacks(new DateTimeCharacteristicCallbacks());
  {
    const char *json = R"({"type":"time", "order":1, "disabled":false, "label":"Time"})";
    NimBLEDescriptor *desc = pCharacteristicTime->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint32_t time = 6 * 60 * 60 + 9 * 60;
  pCharacteristicTime->setValue(time);

  // date32: Editable control for date (uint32_t) characteristics
  // The value represents the number of seconds since the epoch.
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicDate32 = pService->createCharacteristic(
    CHARACTERISTIC_DATE32_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicDate32->setCallbacks(new DateTimeCharacteristicCallbacks());
  {
    const char *json = R"({"type":"date32", "order":2, "disabled":false, "label":"Date 32"})";
    NimBLEDescriptor *desc = pCharacteristicDate32->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint32_t date32 = mktime(&dt);
  pCharacteristicDate32->setValue(date32);

  // date64: Editable control for date (uint64_t) characteristics
  // The value represents the number of seconds since the epoch.
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicDate64 = pService->createCharacteristic(
    CHARACTERISTIC_DATE64_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicDate64->setCallbacks(new DateTimeCharacteristicCallbacks());
  {
    const char *json = R"({"type":"date64", "order":3, "disabled":false, "label":"Date 64"})";
    NimBLEDescriptor *desc = pCharacteristicDate64->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint64_t date64 = mktime(&dt);
  pCharacteristicDate64->setValue((uint8_t *)&date64, sizeof(uint64_t));

  // datetime32: Editable control for datetime (uint32_t) characteristics
  // The value represents the number of seconds since the epoch.
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicDateTime32 = pService->createCharacteristic(
    CHARACTERISTIC_DATETIME32_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicDateTime32->setCallbacks(new DateTimeCharacteristicCallbacks());
  {
    const char *json = R"({"type":"datetime32", "order":4, "disabled":false, "label":"DateTime 32"})";
    NimBLEDescriptor *desc = pCharacteristicDateTime32->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint32_t dateTime32 = mktime(&dtm);
  pCharacteristicDateTime32->setValue(dateTime32);

  // datetime64: Editable control for datetime (uint64_t) characteristics
  // The value represents the number of seconds since the epoch.
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicDateTime64 = pService->createCharacteristic(
    CHARACTERISTIC_DATETIME64_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicDateTime64->setCallbacks(new DateTimeCharacteristicCallbacks());
  {
    const char *json = R"({"type":"datetime64", "order":5, "disabled":false, "label":"DateTime 64"})";
    NimBLEDescriptor *desc = pCharacteristicDateTime64->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint64_t dateTime64 = mktime(&dtm);
  //1735732740;
  pCharacteristicDateTime64->setValue((uint8_t *)&dateTime64, sizeof(uint64_t));

  // Start the BLE service
  pService->start();

  // Start BLE advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  Serial.println("BLE server is running and advertising...");
}

void loop() {
}
