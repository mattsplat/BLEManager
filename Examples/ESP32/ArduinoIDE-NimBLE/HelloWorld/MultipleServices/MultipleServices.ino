// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/HelloWorld/MultipleServices/MultipleServices.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE services
#define SERVICE_UUID                       "01000030-74ee-43ce-86b2-0dde20dcefd6"
#define SERVICE2_UUID                      "01000031-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID   "11000030-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TEXT_UUID           "11000031-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SWITCH_UUID         "11000032-74ee-43ce-86b2-0dde20dcefd6"

// Default UUID mask for the BLE Manager app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by BLE Manager to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID            "2000face-74ee-43ce-86b2-0dde20dcefd6"

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

class StringCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    String value = pCharacteristic->getValue().c_str();
    Serial.print("Received value: ");
    if (!value.isEmpty()) {
      Serial.println(value.c_str());
    }
  }
};

class BooleanCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    String value = pCharacteristic->getValue().c_str();
    Serial.print("Received value: ");
    if (!value.isEmpty()) {
      if (value[0]) {
        Serial.println("true");
      } else {
        Serial.println("false");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Initialize BLE device with a name
  NimBLEDevice::init("Multiple Services Device");

  // Create a BLE server and set its callback class
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create a BLE service. Unlike Bluedroid, NimBLE allocates handles
  // dynamically — no numHandles argument required.
  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE characteristic for service name
  // The value of this characteristic will be displayed as the service name.
  // The "order" value determines the order in which the service appears in the BLE Manager app.
  // Only one "serviceName" characteristic is supported per service.
  // If a service contains multiple "serviceName" characteristics, one may be selected randomly.
  NimBLECharacteristic *pCharacteristicServiceName = pService->createCharacteristic(
    CHARACTERISTIC_SERVICE_NAME_UUID,
    NIMBLE_PROPERTY::READ);

  // Add a custom descriptor used by the BLE Manager app.
  // NimBLE enforces the declared max_len strictly — use the JSON literal's
  // strlen() to avoid wasting RAM and to surface oversize crashes during
  // setup() instead of later via random reads.
  {
    const char *json = R"({"type":"serviceName", "order":1})";
    NimBLEDescriptor *desc = pCharacteristicServiceName->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  // Set an initial value for the characteristic
  pCharacteristicServiceName->setValue("Service 1");

  // Text field: editable control for string characteristics
  // Supports UTF-8 encoding
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicText = pService->createCharacteristic(
    CHARACTERISTIC_TEXT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );

  pCharacteristicText->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"text", "order":1, "disabled":false, "label":"My Text Field Label", "maxBytes": 80})";
    NimBLEDescriptor *desc = pCharacteristicText->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicText->setValue("Hello, World!");
  pService->start();

  // SECOND SERVICE
  // NimBLE handles multiple services naturally — call createService() once per
  // service and start() each one before advertising.
  NimBLEService *pService2 = pServer->createService(SERVICE2_UUID);

  NimBLECharacteristic *pCharacteristicService2Name = pService2->createCharacteristic(
    CHARACTERISTIC_SERVICE_NAME_UUID,
    NIMBLE_PROPERTY::READ);

  {
    const char *json = R"({"type":"serviceName", "order":2})";
    NimBLEDescriptor *desc = pCharacteristicService2Name->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicService2Name->setValue("Service 2");

  // Boolean characteristic
  NimBLECharacteristic *pCharacteristicSwitch = pService2->createCharacteristic(
    CHARACTERISTIC_SWITCH_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );

  pCharacteristicSwitch->setCallbacks(new BooleanCharacteristicCallbacks());

  {
    const char *json = R"({"type":"switch", "order":1, "disabled":false, label:"Switch"})";
    NimBLEDescriptor *desc = pCharacteristicSwitch->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  uint8_t b = false;
  pCharacteristicSwitch->setValue(&b, 1);

  pService2->start();

  // Start BLE advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->addServiceUUID(SERVICE2_UUID);
  pAdvertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  Serial.println("BLE server is running and advertising...");
}

void loop() {
}
