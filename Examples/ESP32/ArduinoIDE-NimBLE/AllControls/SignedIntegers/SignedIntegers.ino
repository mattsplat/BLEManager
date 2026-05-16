// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/SignedIntegers/SignedIntegers.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "00000020-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "10000020-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SINT8_UUID "10000021-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SINT16_UUID "10000022-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SINT32_UUID "10000023-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SINT64_UUID "10000024-74ee-43ce-86b2-0dde20dcefd6"
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

class IntegerCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    Serial.print("Received value: ");

    if (dataLength > 0) {
      switch (dataLength) {
        case sizeof(int8_t):  // 8-bit integer
          {
            int8_t intValue;
            memcpy(&intValue, data, sizeof(int8_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(int16_t):  // 16-bit integer
          {
            int16_t intValue;
            memcpy(&intValue, data, sizeof(int16_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(int32_t):  // 32-bit integer
          {
            int32_t intValue;
            memcpy(&intValue, data, sizeof(int32_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(int64_t):  // 64-bit integer
          {
            int64_t intValue;
            memcpy(&intValue, data, sizeof(int64_t));
            Serial.println(intValue);
          }
          break;
        default:
          Serial.println("Invalid data length!");
      }
    } else {
      Serial.println("Empty value received!");
    }
  }
};

void setup() {
  Serial.begin(115200);

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

  pCharacteristicServiceName->setValue("Signed Integers");

  //// CONTROLS ////

  // sint8: editable control for signed byte characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicSInt8 = pService->createCharacteristic(
    CHARACTERISTIC_SINT8_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicSInt8->setCallbacks(new IntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"sint8", "order":1, "disabled":false, "label":"Signed Byte", "minInt":-70, "maxInt":70})";
    NimBLEDescriptor *desc = pCharacteristicSInt8->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint8_t sint8 = -69;
  pCharacteristicSInt8->setValue(&sint8, sizeof(uint8_t));

  // sint16: Editable control for signed int16 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicSInt16 = pService->createCharacteristic(
    CHARACTERISTIC_SINT16_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicSInt16->setCallbacks(new IntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"sint16", "order":2, "disabled":false, "label":"Signed Int16", "minInt":-7070, "maxInt":7070})";
    NimBLEDescriptor *desc = pCharacteristicSInt16->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  int16_t sint16 = -6969;
  pCharacteristicSInt16->setValue((uint8_t *)&sint16, sizeof(int16_t));

  // sint32: Editable control for signed int32 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicSInt32 = pService->createCharacteristic(
    CHARACTERISTIC_SINT32_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicSInt32->setCallbacks(new IntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"sint32", "order":3, "disabled":false, "label":"Signed Int32", "minInt":-707070, "maxInt":707070})";
    NimBLEDescriptor *desc = pCharacteristicSInt32->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  int32_t sint32 = -696969;
  pCharacteristicSInt32->setValue((uint8_t *)&sint32, sizeof(int32_t));

  // sint64: Editable control for signed int64 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicSInt64 = pService->createCharacteristic(
    CHARACTERISTIC_SINT64_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicSInt64->setCallbacks(new IntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"sint64", "order":4, "disabled":false, "label":"Signed Int64", "minInt":-7070707070, "maxInt":7070707070})";
    NimBLEDescriptor *desc = pCharacteristicSInt64->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  int64_t sint64 = -6969696969;
  pCharacteristicSInt64->setValue((uint8_t *)&sint64, sizeof(int64_t));

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
