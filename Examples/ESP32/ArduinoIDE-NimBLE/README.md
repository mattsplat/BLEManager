# ESP32 examples — NimBLE-Arduino 2.x port

Mirror of [`../ArduinoIDE/`](../ArduinoIDE/) but built on top of
[NimBLE-Arduino 2.x](https://github.com/h2zero/NimBLE-Arduino) instead of
the Bluedroid stack bundled with the ESP32 Arduino core.

Both trees implement the **same BLE Manager Android protocol** — identical
service / characteristic UUIDs, identical descriptor JSON, identical
observable behavior on the app. Pick whichever stack fits your project.

## Why NimBLE?

| Aspect | Bluedroid (`ArduinoIDE/`) | NimBLE (`ArduinoIDE-NimBLE/`) |
|---|---|---|
| Binary size | baseline | typically 30–50% smaller |
| RAM usage | baseline | ~30 KB less |
| CCCD descriptor (`0x2902`) | manual (`BLE2902`) | automatic |
| Service handle count | manual (`numHandles`) | dynamic |
| Maintenance | bundled with arduino-esp32 | active upstream (h2zero) |
| Bluedroid coexistence with Wi-Fi | OK | OK |

NimBLE wins on size / RAM, which matters when a project already pushes
the partition limit (e.g. WiFi + OTA + custom features). For tiny demos
either stack works fine.

## Requirements

- ESP32 Arduino core **3.x** (`esp32:esp32`)
- [`NimBLE-Arduino`](https://github.com/h2zero/NimBLE-Arduino) **2.x** (install via `arduino-cli lib install NimBLE-Arduino` or the IDE Library Manager)

> **Mandatory override:** every sketch declares
> `extern "C" bool btInUse() { return true; }` at file scope. Without it,
> `initArduino()` releases the BT controller memory before NimBLE inits
> and the boot asserts at `npl_freertos_mutex_pend`. This is specific
> to arduino-esp32 3.x — earlier cores did not need it.

## Examples mapping

| Path | What it shows |
|---|---|
| `HelloWorld/HelloWorld/` | Minimal serviceName + read/write text characteristic |
| `HelloWorld/Bonding/` | Static-PIN bonding with encrypted access permissions |
| `HelloWorld/Indications/` | INDICATE (acknowledged notifications) |
| `HelloWorld/LEDControl/` | GPIO output controlled from a boolean characteristic |
| `HelloWorld/MultipleClients/` | Simultaneous connections (NimBLE `setMaxConnections`) |
| `HelloWorld/MultipleServices/` | Multiple services → multiple tabs on the app |
| `AllControls/*` | One sketch per supported control type (Booleans, Button, Color, DateAndTime, Dropdown, Floats, FloatSliders, SignedIntegers, SignedIntegerSliders, Texts, UnsignedIntegers, UnsignedIntegerSliders) |
| `AllControls/BigEndian/*` | Same controls, but characteristic values are big-endian — exercises the app's `bigEndian` JSON flag |
| `Advanced/MultipleMasks/` | Per-characteristic descriptor UUID masks |
| `Advanced/WiFiConfig/` | Wi-Fi provisioning over BLE (production-grade pattern with two services / two tabs) |

## Conversion recipe (one-page summary)

If you already have a Bluedroid sketch following the BLE Manager
convention, the port to NimBLE 2.x is mechanical:

1. Replace the four headers (`BLEDevice.h`, `BLEServer.h`, `BLEUtils.h`,
   `BLE2902.h`) with a single `#include <NimBLEDevice.h>`.
2. Add `extern "C" bool btInUse() { return true; }` at file scope.
3. Prefix every `BLE*` type with `Nim` (`BLEServer` → `NimBLEServer`,
   `BLECharacteristic` → `NimBLECharacteristic`, etc.).
4. Replace `BLECharacteristic::PROPERTY_*` with `NIMBLE_PROPERTY::*`.
5. `createService(uuid, numHandles)` → `createService(uuid)` (handles
   are dynamic in NimBLE).
6. Replace `new BLEDescriptor(...); addDescriptor(...)` with
   `pChar->createDescriptor(uuid, NIMBLE_PROPERTY::READ, exact_len)` and
   `setValue((uint8_t*)buf, exact_len)`. Use `strlen()` of the actual
   JSON literal — never an over-sized buffer (NimBLE enforces the
   declared `max_len`).
7. Drop any `BLE2902` instantiation — NimBLE adds the CCCD descriptor
   automatically for `NOTIFY`/`INDICATE` characteristics.
8. Update callback signatures:
   - `onConnect(BLEServer*)` → `onConnect(NimBLEServer*, NimBLEConnInfo&)`
   - `onDisconnect(BLEServer*)` → `onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason)`
   - `onWrite(BLECharacteristic*)` → `onWrite(NimBLECharacteristic*, NimBLEConnInfo&)`
9. Bonding: replace `BLESecurity` + `setStaticPIN` + `setAuthenticationMode`
   with `NimBLEDevice::setSecurityAuth(true, true, true)` +
   `NimBLEDevice::setSecurityPasskey(...)` +
   `NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY)`. Express
   encrypted access via `NIMBLE_PROPERTY::READ_ENC` / `WRITE_ENC`
   merged into the `createCharacteristic` property bitmask —
   NimBLE-Arduino 2.x has no `setAccessPermissions()` method.
10. `MultipleClients`: there is no runtime `setMaxConnections()` in
   NimBLE-Arduino 2.x — the maximum is the compile-time
   `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` (default 3). To accept additional
   clients while one is already connected, call
   `NimBLEDevice::startAdvertising()` again inside `onConnect`.
11. `setScanResponse(bool)` was renamed to `enableScanResponse(bool)`.
   `setScanResponseData()` is a different API for custom payloads.

The descriptor JSON (`{"type":"text", ...}`) and all UUIDs (including
the `####face` mask) are stack-agnostic — copy them unchanged.

## Validating the examples

```bash
./.tools/compile-all.sh
```

Compiles every sketch in this tree against `esp32:esp32:esp32`. Exits
non-zero on any failure.
