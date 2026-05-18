// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/BigEndian/FloatSlidersBigEndian/FloatSlidersBigEndian.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

/*
ESP32 transmits data in Little-Endian byte order. To emulate Big-Endian byte order, we must reverse the byte order.
*/

// UUID for the BLE service
#define SERVICE_UUID "000000C0-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "100000C0-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_HALF_UUID "100000C1-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_FLOAT_UUID "100000C2-74ee-43ce-86b2-0dde20dcefd6"
// Default UUID mask for the BLE Manager app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by BLE Manager to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID "2000face-74ee-43ce-86b2-0dde20dcefd6"

float byteswap32(float x) {
  uint32_t temp;
  memcpy(&temp, &x, sizeof(temp));

  temp = ((temp & 0xFF) << 24) | ((temp >> 8 & 0xFF) << 16) | ((temp >> 16 & 0xFF) << 8) | ((temp >> 24 & 0xFF));

  float result;
  memcpy(&result, &temp, sizeof(result));
  return result;
}

double byteswap64(double x) {
  uint64_t temp;
  memcpy(&temp, &x, sizeof(temp));

  temp = ((temp & 0xFF) << 56) | ((temp >> 8 & 0xFF) << 48) | ((temp >> 16 & 0xFF) << 40) | ((temp >> 24 & 0xFF) << 32) | ((temp >> 32 & 0xFF) << 24) | ((temp >> 40 & 0xFF) << 16) | ((temp >> 48 & 0xFF) << 8) | ((temp >> 56 & 0xFF));

  double result;
  memcpy(&result, &temp, sizeof(result));
  return result;
}

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

//Big Endian
class FloatCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    Serial.print("Received value: ");

    if (dataLength > 0) {
      if (dataLength == 2) {  // 16-bit halfFloat
        uint16_t halfValue = (data[0] << 8) | data[1];
        float floatValue = halfToFloat(halfValue);  //halfFloat -> float
        Serial.println(floatValue);
      } else if (dataLength == 4) {  // 32-bit Float
        uint32_t intValue = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        float floatValue;
        memcpy(&floatValue, &intValue, sizeof(float));
        Serial.println(floatValue);
      } else if (dataLength == 8) {  // 64-bit Double
        uint64_t intValue = (static_cast<uint64_t>(data[0]) << 56) | (static_cast<uint64_t>(data[1]) << 48) | (static_cast<uint64_t>(data[2]) << 40) | (static_cast<uint64_t>(data[3]) << 32) | (static_cast<uint64_t>(data[4]) << 24) | (static_cast<uint64_t>(data[5]) << 16) | (static_cast<uint64_t>(data[6]) << 8) | static_cast<uint64_t>(data[7]);
        double doubleValue;
        memcpy(&doubleValue, &intValue, sizeof(double));
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
  // If you add or remove characteristics, it may be necessary to forget the device (if paired)
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.
  // Alternatively you can try Clear GATT cahce from device menu un MingleApp

  // Create a BLE characteristic for service name
  // The value of this characteristic will be displayed as the service name.
  // The "order" value determines the order in which the service appears in the BLE Manager app.
  // Only one "serviceName" characteristic is supported per service.
  // If a service contains multiple "serviceName" characteristics, one may be selected randomly.

  NimBLECharacteristic *pCharacteristicServiceName = pService->createCharacteristic(
    CHARACTERISTIC_SERVICE_NAME_UUID,
    NIMBLE_PROPERTY::READ);

  {
    const char *json = R"({"type":"serviceName", "order":1})";
    NimBLEDescriptor *desc = pCharacteristicServiceName->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  // Set an initial value for the characteristic
  pCharacteristicServiceName->setValue("Big Endian Float Sliders");

  //// CONTROLS ////
  // If you add or remove characteristics, it may be necessary to forget the device (if paired)
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.
  // Alternatively you can try Clear GATT cahce from device menu un MingleApp

  // HalfSlider: editable slider control for float16 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicHalf = pService->createCharacteristic(
    CHARACTERISTIC_HALF_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicHalf->setCallbacks(new FloatCharacteristicCallbacks());
  {
    const char *json = R"({"type":"halfsliderbe", "order":1, "disabled":false, "label":"Float 16", "minFloat": 0, "maxFloat": 75, "stepFloat": 0.1})"; //Defaults: minFloat = 0, maxFloat = 100, stepFloat: 1
    NimBLEDescriptor *desc = pCharacteristicHalf->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint16_t h = 0x4052; //50
  pCharacteristicHalf->setValue((uint8_t *)&h, sizeof(uint16_t));

  // FloatSlider: editable slider control for float32 characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicFloat = pService->createCharacteristic(
    CHARACTERISTIC_FLOAT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicFloat->setCallbacks(new FloatCharacteristicCallbacks());
  {
    const char *json = R"({"type":"floatsliderbe", "order":2, "disabled":false, label:"Float 32", "minFloat": -50, "maxFloat": 50, "stepFloat": 1})"; //Defaults: minFloat = 0, maxFloat = 100, stepFloat: 1
    NimBLEDescriptor *desc = pCharacteristicFloat->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  float f = byteswap32(0);
  pCharacteristicFloat->setValue(f);

  // Start the BLE service
  pService->start();

  // Start BLE advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // Advertise the service UUID
  pAdvertising->enableScanResponse(true);      // Enable scan response
  NimBLEDevice::startAdvertising();

  Serial.println("BLE server is running and advertising...");
}

void loop() {
}
