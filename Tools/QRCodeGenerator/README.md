# QR Code Generator

A single static page that generates QR codes for adding devices in the BLE Manager app (*Add New Device → Scan QR Code*).

Open `index.html` in a browser. It works offline and needs no build step.

- Enter the device's Bluetooth LE address, a name, or both. See [Adding a Device with a QR Code](../../README.md#adding-a-device-with-a-qr-code) for how the app uses each.
- Choose a caption to print under the code: the name, the address, both, or none.
- **Download PNG** (high resolution), **Download SVG** (vector, for label software), or **Print** at a chosen width in millimeters.

`qrcode.js` is [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator) 1.4.4 by Kazuhiko Arase (MIT license), included unmodified.
