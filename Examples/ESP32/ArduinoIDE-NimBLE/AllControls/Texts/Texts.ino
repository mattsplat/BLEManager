// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/AllControls/Texts/Texts.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID                       "00000000-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID   "10000000-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TITLE_VIEW_UUID     "10000001-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TEXT_VIEW_UUID      "10000002-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_TEXT_UUID           "10000003-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_PASSWORD_UUID       "10000004-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_PIN_UUID            "10000005-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_RICH_TEXT_VIEW_UUID "10000006-74ee-43ce-86b2-0dde20dcefd6"
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

  pCharacteristicServiceName->setValue("Texts");

  //// CONTROLS ////

  // Title: read-only large text
  NimBLECharacteristic *pCharacteristicTitleView = pService->createCharacteristic(
    CHARACTERISTIC_TITLE_VIEW_UUID,
    NIMBLE_PROPERTY::READ
  );

  {
    const char *json = R"({"type":"titleView", "order":1, "disabled":false})";  // Control is always read-only. "Disabled" has only a visual effect.
    NimBLEDescriptor *desc = pCharacteristicTitleView->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicTitleView->setValue("Large read-only text");

  // TextView: read-only regular-sized text
  NimBLECharacteristic *pCharacteristicTextView = pService->createCharacteristic(
    CHARACTERISTIC_TEXT_VIEW_UUID,
    NIMBLE_PROPERTY::READ
  );

  {
    const char *json = R"({"type":"textView", "order":2, "disabled":false})";  // Control is always read-only. "Disabled" has only a visual effect.
    NimBLEDescriptor *desc = pCharacteristicTextView->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicTextView->setValue("Read-only text");

  // RichTextView: read-only regular-sized text
  NimBLECharacteristic *pCharacteristicRichTextView = pService->createCharacteristic(
    CHARACTERISTIC_RICH_TEXT_VIEW_UUID,
    NIMBLE_PROPERTY::READ
  );

  {
    const char *json = R"({"type":"richTextView", "order":3, "disabled":false})";  // Control is always read-only. "Disabled" has only a visual effect.
    NimBLEDescriptor *desc = pCharacteristicRichTextView->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicRichTextView->setValue(R"({"text":"Colored Text", "color":"#000000", "background":"#F2E605", "title":true})" //color format can be #AARRGGBB or #RRGGBB
  );

  // Text field: editable control for string characteristics
  // Supports UTF-8 encoding
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicText = pService->createCharacteristic(
    CHARACTERISTIC_TEXT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );
  pCharacteristicText->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"text", "order":4, "disabled":false, "label":"Text Field Label", "maxBytes": 30})"; //maxBytes specifies the maximum number of bytes. It can be set up to 512 bytes. The default value is 512 bytes.
    NimBLEDescriptor *desc = pCharacteristicText->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicText->setValue("Text value");

  // Password field: editable control password string characteristics
  // Supports UTF-8 encoding
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicPassword = pService->createCharacteristic(
    CHARACTERISTIC_PASSWORD_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );

  pCharacteristicPassword->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"password", "order":5, "disabled":false, label:"Pasword Field Label", "maxBytes": 30})"; //maxBytes specifies the maximum number of bytes. It can be set up to 512 bytes. The default value is 512 bytes.
    NimBLEDescriptor *desc = pCharacteristicPassword->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicPassword->setValue("");


  // PIN field: editable control PIN string characteristics
  // If the characteristic is not writable, the "disabled" property is ignored, and the control remains disabled.
  NimBLECharacteristic *pCharacteristicPIN = pService->createCharacteristic(
    CHARACTERISTIC_PIN_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );

  pCharacteristicPIN->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"pin", "order":6, "disabled":false, label:"PIN Field Label", "maxBytes": 30})"; //maxBytes specifies the maximum number of bytes. It can be set up to 512 bytes. The default value is 512 bytes.
    NimBLEDescriptor *desc = pCharacteristicPIN->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pCharacteristicPIN->setValue("");

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
