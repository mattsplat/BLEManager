// NimBLE-Arduino 2.x port of Examples/ESP32/ArduinoIDE/Advanced/WiFiConfig/WiFiConfig.ino
// Same service UUIDs, characteristic UUIDs, descriptor JSON, and observable
// behavior on the BLE Manager Android app. See ../../README.md for conversion notes.

// SET MEMORY PROFILE WITH 2MB APP

#include <WiFi.h>
#include <NimBLEDevice.h>

// Mandatory with arduino-esp32 3.x — without it, initArduino() releases the BT
// controller memory before NimBLE init and the boot asserts at
// `npl_freertos_mutex_pend (mu->handle null)`.
extern "C" bool btInUse() { return true; }

// UUID for the BLE service
#define SERVICE_UUID "00100000-74ee-43ce-86b2-0dde20dcefd6"
// UUIDs for BLE characteristics
#define CHARACTERISTIC_SERVICE_NAME_UUID "10100000-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_SSID_UUID "10100001-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_PASSWORD_UUID "10100002-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_STATUS_UUID "10100003-74ee-43ce-86b2-0dde20dcefd6"
#define CHARACTERISTIC_CONNECT_ACTION_UUID "10100004-74ee-43ce-86b2-0dde20dcefd6"

// Default UUID mask for the BLE Manager app is ####face-####-####-####-############
// The segment "face" (case-insensitive) is used by BLE Manager to identify descriptors
#define CUSTOM_DESCRIPTOR_UUID "2000face-74ee-43ce-86b2-0dde20dcefd6"

String ssid = "";
String password = "";
// volatile: written from the BLE callback (BLE task), read in loop() (Arduino task).
// WiFi.begin() must NOT be called inside a BLE callback — it would stall the radio.
volatile bool connect = false;
NimBLECharacteristic *pCharacteristicStatus = NULL;

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
    NimBLEUUID receivedUUID = pCharacteristic->getUUID();
    if (receivedUUID.equals(NimBLEUUID(CHARACTERISTIC_SSID_UUID))) {
      ssid = pCharacteristic->getValue().c_str();
    } else if (receivedUUID.equals(NimBLEUUID(CHARACTERISTIC_PASSWORD_UUID))) {
      password = pCharacteristic->getValue().c_str();
    }
  }
};

class ButtonCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    NimBLEUUID receivedUUID = pCharacteristic->getUUID();
    if (receivedUUID.equals(NimBLEUUID(CHARACTERISTIC_CONNECT_ACTION_UUID))) {
      // Set flag only — actual WiFi.begin() happens in loop() to avoid
      // blocking the BLE radio inside the callback.
      connect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Initialize BLE device with a name
  NimBLEDevice::init("WiFi Device");

  // Create a BLE server and set its callback class
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create a BLE service. Unlike Bluedroid, NimBLE allocates handles
  // dynamically — no numHandles argument required.
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  // If you add or remove characteristics, it may be necessary to forget the device
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.

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
  // Only one descriptor matching the mask is supported per characteristic.
  // If multiple descriptors match, one may be selected randomly.
  {
    const char *json = R"({"type":"serviceName", "order":1})";
    NimBLEDescriptor *desc = pCharacteristicServiceName->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  // Set an initial value for the characteristic
  pCharacteristicServiceName->setValue("Settings");

  //// CONTROLS ////
  // If you add or remove characteristics, it may be necessary to forget the device
  // in the Bluetooth settings and re-pair it on Android for changes to take effect.

  // SSID
  NimBLECharacteristic *pCharacteristicSSID = pService->createCharacteristic(
    CHARACTERISTIC_SSID_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  pCharacteristicSSID->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"text", "order":1, "disabled":false, "label":"SSID", "maxBytes": 32})";
    NimBLEDescriptor *desc = pCharacteristicSSID->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  pCharacteristicSSID->setValue("");

  // PASSWORD
  NimBLECharacteristic *pCharacteristicPassword = pService->createCharacteristic(
    CHARACTERISTIC_PASSWORD_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  pCharacteristicPassword->setCallbacks(new StringCharacteristicCallbacks());

  {
    const char *json = R"({"type":"password", "order":2, "disabled":false, "label":"Password", "maxBytes": 64})";
    NimBLEDescriptor *desc = pCharacteristicPassword->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  pCharacteristicPassword->setValue("");

  // STATUS — INDICATE with automatic CCCD (NimBLE creates 0x2902 automatically;
  // no BLE2902 instantiation needed).
  pCharacteristicStatus = pService->createCharacteristic(
    CHARACTERISTIC_STATUS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE);

  {
    const char *json = R"({"type":"textView", "order":3, "disabled":false})";
    NimBLEDescriptor *desc = pCharacteristicStatus->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }
  pCharacteristicStatus->setValue("Not Connected");

  // BUTTON
  NimBLECharacteristic *pCharacteristicConnectButton = pService->createCharacteristic(
    CHARACTERISTIC_CONNECT_ACTION_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

  pCharacteristicConnectButton->setCallbacks(new ButtonCharacteristicCallbacks());

  {
    const char *json = R"({"type":"button", "order":4, "label":"Connect", "disabled":false})";
    NimBLEDescriptor *desc = pCharacteristicConnectButton->createDescriptor(
      CUSTOM_DESCRIPTOR_UUID, NIMBLE_PROPERTY::READ, strlen(json));
    desc->setValue((uint8_t*)json, strlen(json));
  }

  pService->start();

  // Start BLE advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // Advertise the service UUID
  pAdvertising->enableScanResponse(true);      // Enable scan response
  NimBLEDevice::startAdvertising();

  Serial.println("BLE server is running and advertising...");
}

void connectToWiFi() {
  if (connect) {
    connect = false;
    if (!ssid.isEmpty()) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Connecting: ");
        Serial.println(ssid);

        pCharacteristicStatus->setValue("Connecting...");
        pCharacteristicStatus->indicate();
        WiFi.begin(ssid, password);

        if (WiFi.waitForConnectResult(10000) == WL_CONNECTED) {
          Serial.println("Connected:");
          Serial.print("IP: ");
          Serial.println(WiFi.localIP());
          pCharacteristicStatus->setValue(String("IP: ") + WiFi.localIP().toString());
        } else {
          Serial.println("Connection Failed");
          pCharacteristicStatus->setValue("Connection Failed");
        }

        pCharacteristicStatus->indicate();
      }
    } else {
      pCharacteristicStatus->setValue("Please set the SSID");
      pCharacteristicStatus->indicate();
    }
  }
}


void loop() {
  connectToWiFi();
}
