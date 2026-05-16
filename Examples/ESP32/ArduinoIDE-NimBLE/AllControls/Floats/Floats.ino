// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/Floats/Floats.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "00000060-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "10000060-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_HALF_UUID "10000061-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_FLOAT_UUID "10000062-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_DOUBLE_UUID "10000063-74ee-43ce-86b2-0dde20dcefd6"
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

class FloatCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    Serial.print("Received value: ");

    if (dataLength > 0) {
      if (dataLength == 2) {  // 16-bit halfFloat
        uint16_t halfValue = data[0] | (data[1] << 8);
        float floatValue = halfToFloat(halfValue);  //halfFloat -> float
        Serial.println(floatValue);
      } else if (dataLength == 4) {  // 32-bit Float
        float floatValue;
        memcpy(&floatValue, data, sizeof(float));
        Serial.println(floatValue);
      } else if (dataLength == 8) {  // 64-bit Double
        double doubleValue;
        memcpy(&doubleValue, data, sizeof(double));
        Serial.println(doubleValue);
      } else {
        Serial.println("Invalid data length for floating-point value!");
      }
    } else {
      Serial.println("Empty value received!");
    }
  }

  float halfToFloat(uint16_t half) {
    uint16_t sign = (half & 0x8000) >> 15;
    uint16_t exponent = (half & 0x7C00) >> 10;
    uint16_t mantissa = half & 0x03FF;

    if (exponent == 0) {  // Subnormal number or zero
      if (mantissa == 0) {
        return sign ? -0.0f : 0.0f;
      } else {
        return (sign ? -1.0f : 1.0f) * (mantissa / 1024.0f) * pow(2, -14);
      }
    } else if (exponent == 0x1F) {  // Infinity or NaN
      return mantissa ? NAN : (sign ? -INFINITY : INFINITY);
    } else {  // Normal number
      return (sign ? -1.0f : 1.0f) * (1.0f + (mantissa / 1024.0f)) * pow(2, exponent - 15);
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

  pCharacteristicServiceName->setValue("Floats");

  //// CONTROLS ////

  // Half: editable control for float16 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicHalf = pService->createCharacteristic(
    CHARACTERISTIC_HALF_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicHalf->setCallbacks(new FloatCharacteristicCallbacks());
  {
    const char *json = R"({"type":"half", "order":1, "disabled":false, "label":"Float 16", "minFloat": -10, "maxFloat": 10})";
    NimBLEDescriptor *desc = pCharacteristicHalf->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint16_t h = 0x46E6;
  pCharacteristicHalf->setValue((uint8_t *)&h, sizeof(uint16_t));

  // Float: editable control for float32 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicFloat = pService->createCharacteristic(
    CHARACTERISTIC_FLOAT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  pCharacteristicFloat->setCallbacks(new FloatCharacteristicCallbacks());
  {
    const char *json = R"({"type":"float", "order":2, "disabled":false, label:"Float 32", "minFloat": -20, "maxFloat": 20})";
    NimBLEDescriptor *desc = pCharacteristicFloat->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  float f = 6.9;
  pCharacteristicFloat->setValue(f);

  // Double: editable control for float64 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicDouble = pService->createCharacteristic(
    CHARACTERISTIC_DOUBLE_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  pCharacteristicDouble->setCallbacks(new FloatCharacteristicCallbacks());
  {
    const char *json = R"({"type":"double", "order":3, "disabled":false, label:"Float 64", "minFloat": -30, "maxFloat": 30})";
    NimBLEDescriptor *desc = pCharacteristicDouble->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  double d = 6.9;
  pCharacteristicDouble->setValue(d);

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
