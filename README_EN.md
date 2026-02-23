# ESP32_QC3_CTL

QuickCharge 3.0 control library for ESP32.

This repository also includes a WebUI sample for M5Stack ATOM S3 (operable in AP mode).

## Hardware Design Information

The hardware design information used in this project is published on OSHWLab.

[QuickCharge adapter for M5ATOMS3](https://oshwlab.com/tomorrow56/m5atom_lipo_copy_copy)

## Installation

- Place this `ESP32_QC3_CTL` folder in the `libraries` directory under your Arduino IDE sketchbook folder.

## Usage

- `#include <ESP32_QC3_CTL.h>`
- `ESP32_QC3_CTL qc3(dp_h, dp_l, dm_h, dm_l, vbus_det, out_en);`
- `qc3.begin();`

Register any ADC pins with `addAdcPin(pin, attenuation)` to automatically apply atten settings during `begin()`.

## Library API Specification (ESP32_QC3_CTL)

### Enum Types

- `QC_STATE`
  - `QC_HIZ` / `QC_0V` / `QC_600mV` / `QC_3300mV`
  - D+/D- applied voltage states
- `HOST_PORT_TYPE`
  - `BC_NA` / `BC_DCP` / `QC3`
  - Connected port type
- `QC_VOLTAGE_MODE`
  - `QC_5V` / `QC_9V` / `QC_12V` / `QC_20V` / `QC_VAR`
  - VBUS output voltage mode

### Constructor

- `ESP32_QC3_CTL(uint8_t dp_h, uint8_t dp_l, uint8_t dm_h, uint8_t dm_l, uint8_t vbus_det, uint8_t out_en = 0)`
  - **dp_h/dp_l/dm_h/dm_l**: D+/D- control GPIO pins
  - **vbus_det**: VBUS detection (ADC) GPIO pin
  - **out_en**: Output ON/OFF control GPIO pin (set to `0` if unused)

### Initialization

- `bool begin()`
  - Performs pin initialization, ADC resolution setting, and applies atten settings for registered ADC pins.
  - **Returns**: `true` on success

### Charger (Port Type) Detection

- `uint8_t detect_Charger()`
  - Determines BC1.2 DCP/QC3.0.
  - **Returns**: `BC_NA` / `BC_DCP` / `QC3`
  - **Note**: Modifies D+/D- states internally. For QC3 detection, temporarily attempts 20V setting for Class B determination.

### Voltage Setting

- `bool set_VBUS(uint8_t mode)`
  - Sets VBUS output voltage mode.
  - **mode**: `QC_5V` / `QC_9V` / `QC_12V` / `QC_20V` / `QC_VAR`
  - **Returns**: `true` if setting successful
  - **Note**: Only valid when `QC3` is detected by `detect_Charger()` (returns `false` otherwise).

### Variable Mode (QC_VAR)

- `void var_inc()`
- `void var_dec()`
  - Increases/decreases by 200mV steps in `QC_VAR` mode.
  - **Note**: No effect in modes other than `QC_VAR`.

### Getter Functions

- `uint16_t getVoltage()`
  - Returns current VBUS setting value.
  - **Unit**: mV
  - **Note**: This is the "setting value," not actual measurement. For actual measurement, use `readVoltage(vbus_det)` etc.

- `uint8_t getHostType()`
  - Returns host type result from `detect_Charger()`.

- `bool getUseClassB()`
  - Returns internal Class B usage determination result.

### ADC/Voltage Reading

- `float readVoltage(uint16_t Vread)`
  - Converts ADC raw value to voltage (V).

- `float readVoltage(uint8_t pin, uint16_t Vread)`
  - Converts ADC raw value for specified pin to voltage (V).

- `float readVoltage(uint8_t pin)`
  - Performs `analogRead(pin)` and returns voltage (V).

**Note (ESP32 Core/IDF Differences)**:
Depending on the environment (Arduino-ESP32 core version), ADC calibration APIs may not be available, so some environments fall back to simple conversion.

### ADC Pin Registration

- `bool addAdcPin(uint8_t pin)`
- `bool addAdcPin(uint8_t pin, uint8_t attenuation)`
  - Registration for automatic atten setting application during `begin()`.
  - **attenuation**: For ESP32 environment, `ADC_11db` etc. (equivalent to Arduino-ESP32's `adc_attenuation_t`)
  - **Returns**: `true` if registration successful

### Direct D+/D- Operations

- `void set_DP(uint8_t state)`
- `void set_DM(uint8_t state)`
  - Sets D+/D- to `QC_STATE` equivalent state.
  - **Note**: Normally intended for use via `set_VBUS()`/`detect_Charger()`.

## AtomS3_QC3_WebUI (WebUI Sample)

`examples/AtomS3_QC3_WebUI/AtomS3_QC3_WebUI.ino` is a sample where ATOM S3 starts as an access point (AP) and allows output voltage and ON/OFF control from a browser.

<img src="img/atomS3_qc3_webui.png" alt="AtomS3_QC3_WebUI" width="300px">

### Connection

- After ATOM S3 boots, connect to Wi-Fi `ATOMS3_AP`
- Access `http://192.168.4.1/` in your browser

### Pin Definitions

Refer to `examples/AtomS3_QC3_WebUI/PinDefinitions.h`.

### HTTP Endpoints

- `/` : WebUI
- `/voltage?value=5|9|12|20` : Fixed voltage
- `/offset?value=200|-200` : Variable mode (QC_VAR) ±200mV
- `/toggle?state=on|off` : Output ON/OFF
- `/state` : Current output ON/OFF state (`on`/`off`)
- `/current` : Measured output voltage (VBUS mV calculated from VBUS_DET ADC reading)
- `/use_class_b` : Class B availability (`true`/`false`)

### Output Voltage (Measured) Notes

`/current` calculates VBUS(mV) by multiplying VBUS_DET ADC voltage with voltage divider ratio.
The sample uses correction factor `7.67` assuming resistor divider `100kΩ/15kΩ`.
If your voltage divider resistors differ, modify the coefficient in `examples/AtomS3_QC3_WebUI/WebUI.cpp` to match your environment.

### ATOM S3 Body Button

Pressing the body button (`BtnA`) toggles output ON/OFF.
The WebUI periodically fetches `/state` to synchronize display.

### Startup OUT_EN Glitch

To mitigate the phenomenon where `OUT_EN` momentarily turns ON immediately after startup, the code first sets it to LOW via `INPUT_PULLDOWN` at the beginning of `setup()` before switching to output mode.
For complete suppression including bootloader phase behavior, consider hardware measures such as adding a pull-down resistor to `OUT_EN`.

## Folder Structure

```
ESP32_QC3_CTL/
├── src/                           # Library source
│   ├── ESP32_QC3_CTL.h           # Header file
│   └── ESP32_QC3_CTL.cpp         # Implementation file
├── examples/                      # Sample sketches
│   ├── DetectCharger/
│   │   └── DetectCharger.ino      # Basic charger detection sample
│   ├── AtomS3_QC3_WebUI/
│   │   ├── AtomS3_QC3_WebUI.ino   # ATOM S3 WebUI sample (AP mode)
│   │   ├── WebUI.cpp             # WebUI implementation
│   │   └── PinDefinitions.h      # Pin definitions
│   ├── M5Stack_QC3_test/
│   │   └── M5Stack_QC3_test.ino   # M5Stack basic test sketch
│   └── M5Stack_QC3trigger/
│       ├── M5Stack_QC3trigger.ino # M5Stack button GUI (recommended)
│       └── README.md              # Detailed documentation
├── img/                          # Image resources
├── LICENSE                       # License file
├── README.md                     # This file (Japanese)
└── library.properties            # Arduino library information
```

## License

Copyright (c) 2025 @tomorrow56
MIT License
