# AtomS3_QC3_WebUI

WebUI sample for controlling QuickCharge 3.0 output with M5Stack ATOM S3.
ATOM S3 starts as an access point (AP), allowing you to control output voltage (5/9/12/20V, variable ±200mV) and output ON/OFF from a browser.

## Requirements

- Arduino IDE
- Board: ESP32S3 series (ATOM S3)
- Libraries:
  - M5Unified
  - FastLED
  - ESP32_QC3_CTL from this repository

## Wiring/Pins

Defined in `PinDefinitions.h`.

- `DP_H`/`DP_L`/`DM_H`/`DM_L`: QC3 control (D+/D-)
- `VBUS_DET`: VBUS detection (ADC)
- `OUT_EN`: Output ON/OFF
- `LED_DATA_PIN`: ATOM S3 built-in LED

## Usage

1. Open `AtomS3_QC3_WebUI.ino` in Arduino IDE
2. Select board/port and upload
3. After startup, connect to Wi-Fi `ATOMS3_AP` (password: `01234567`)
4. Open `http://192.168.4.1/` in your browser

## Operation

- **WebUI buttons**: Voltage settings, ±200mV, ON/OFF
- **ATOM S3 body button (BtnA)**: Output ON/OFF toggle
  - WebUI periodically fetches `/state` to synchronize display

## HTTP API

WebUI uses the following endpoints.

- `/` : WebUI (HTML)
- `/voltage?value=5|9|12|20` : Set to fixed voltage
- `/offset?value=200|-200` : ±200mV in variable mode (QC_VAR)
- `/toggle?state=on|off` : Output ON/OFF
- `/state` : Current output ON/OFF state (`on`/`off`)
- `/current` : Measured output voltage (mV)
- `/use_class_b` : Class B availability (`true`/`false`)

## Output Voltage (Measured) Conversion

`/current` reads VBUS_DET ADC voltage and multiplies by voltage divider ratio to calculate VBUS(mV).

- Default correction factor: `7.67` (assuming resistor divider 100kΩ/15kΩ)
- Change location: `VBUS_DIVIDER_RATIO` in `WebUI.cpp`

If your actual voltage divider resistors differ, adjust this factor to match your environment.

## Startup OUT_EN Glitch

To mitigate the phenomenon where `OUT_EN` momentarily turns ON immediately after startup, the code first sets it to LOW via `INPUT_PULLDOWN` at the beginning of `setup()` before switching to output mode.
For complete suppression including bootloader phase behavior, consider hardware measures such as adding a pull-down resistor to `OUT_EN`.
