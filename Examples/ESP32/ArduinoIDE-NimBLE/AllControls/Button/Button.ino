// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/Button/Button.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "00000080-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "10000080-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_BUTTON_UUID "10000081-74ee-43ce-86b2-0dde20dcefd6"
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

class UnsignedInt8CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    const uint8_t *data = pCharacteristic->getValue().data();
    size_t dataLength = pCharacteristic->getValue().size();

    Serial.print("Received value: ");

    if (dataLength > 0) {
      if (dataLength == sizeof(uint8_t)) {
        uint8_t intValue;
        memcpy(&intValue, data, sizeof(uint8_t));
        Serial.println(intValue);
      } else {
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

  pCharacteristicServiceName->setValue("Button");

  //// CONTROLS ////

  // button control
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicButton = pService->createCharacteristic(
    CHARACTERISTIC_BUTTON_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristicButton->setCallbacks(new UnsignedInt8CharacteristicCallbacks());
  {
    const char *json = R"({"type":"button", "order":1, "disabled":false, "label":"Button"})";
    NimBLEDescriptor *desc = pCharacteristicButton->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  uint8_t uint8 = 0;
  pCharacteristicButton->setValue(&uint8, sizeof(uint8_t));

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
