// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/UnsignedIntegerSliders/UnsignedIntegerSliders.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "000000E0-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "100000E0-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_UINT8_UUID        "100000E1-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_UINT16_UUID       "100000E2-74ee-43ce-86b2-0dde20dcefd6"
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

class UnsignedIntegerCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    Serial.print("Received value: ");

    if (dataLength > 0) {
      switch (dataLength) {
        case sizeof(uint8_t):  // 8-bit unsigned integer
          {
            uint8_t intValue;
            memcpy(&intValue, data, sizeof(uint8_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(uint16_t):  // 16-bit unsigned integer
          {
            uint16_t intValue;
            memcpy(&intValue, data, sizeof(uint16_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(uint32_t):  // 32-bit unsigned integer
          {
            uint32_t intValue;
            memcpy(&intValue, data, sizeof(uint32_t));
            Serial.println(intValue);
          }
          break;
        case sizeof(uint64_t):  // 64-bit unsigned integer
          {
            uint64_t intValue;
            memcpy(&intValue, data, sizeof(uint64_t));
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

  pCharacteristicServiceName->setValue("Unsigned Integer Sliders");

  //// CONTROLS ////

  // uint8slider: editable slider control for unsigned byte characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicUInt8 = pService->createCharacteristic(
    CHARACTERISTIC_UINT8_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicUInt8->setCallbacks(new UnsignedIntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"uint8slider", "order":1, "disabled":false, "label":"Unsigned Byte", "minInt":0, "maxInt":100, "stepInt":1})"; //Defaults: minInt = 0, maxInt = 100, stepInt: 1
    NimBLEDescriptor *desc = pCharacteristicUInt8->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint8_t uint8 = 50;
  pCharacteristicUInt8->setValue(&uint8, sizeof(uint8_t));

  // uint16slider: Editable slider control for unsigned int16 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicUInt16 = pService->createCharacteristic(
    CHARACTERISTIC_UINT16_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicUInt16->setCallbacks(new UnsignedIntegerCharacteristicCallbacks());
  {
    const char *json = R"({"type":"uint16slider", "order":2, "disabled":false, "label":"Unsigned Int16", "minInt":100, "maxInt":200, "stepInt":2})"; //Defaults: minInt = 0, maxInt = 100, stepInt: 1
    NimBLEDescriptor *desc = pCharacteristicUInt16->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint16_t uint16 = 100;
  pCharacteristicUInt16->setValue((uint8_t *)&uint16, sizeof(uint16_t));

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
