# M5Stack QC3 Trigger

QuickCharge 3.0 Voltage Controller for M5Stack  
Control QC3.0 compatible charger output voltage using M5Stack buttons.

## Features

- **Fixed Voltage Mode**: 5V / 9V / 12V / 20V (Class B compatible only)
- **Variable Voltage Mode**: Adjust voltage in 200mV steps
- **QC3.0 Detection**: Automatically detects and displays charger type
- **Voltage & Current Measurement**: Real-time display of VBUS output voltage and current
- **QC Capabilities Display**: Shows connected charger's supported voltage list
- **Output ON/OFF Control**: FET gate control via VBUSEN pin

## Hardware Configuration

### M5Stack Basic/Gray/Core Pin Assignment

| Signal | Pin | Function |
|--------|-----|----------|
| DP_H | 13 | D+ HIGH (10kΩ) |
| DP_L | 16 | D+ LOW (2.2kΩ) |
| DM_H | 26 | D- HIGH (10kΩ) |
| DM_L | 17 | D- LOW (2.2kΩ) |
| VBUS_I | 35 | VBUS voltage detection (ADC) |
| VI_I | 36 | Current detection (ACS712xLCTR-05B) |
| VBUSEN_O | 2 | Output enable (FET gate) |

### M5Stack Core2 Pin Assignment (commented out)

| Signal | Pin |
|--------|-----|
| DP_H | 19 |
| DP_L | 13 |
| DM_H | 26 |
| DM_L | 14 |
| VBUSEN_O | 32 |

## Operation

### Normal Mode

| Button | Function |
|--------|----------|
| **A** | CAP (QC Capabilities display) / Voltage DOWN |
| **B short press** | Output ON/OFF |
| **B long press (2s)** | VAR mode enter/exit |
| **C** | Voltage UP |

### VAR Mode (Variable Voltage)

| Button | Function |
|--------|----------|
| **A** | -200mV (long-press repeat supported) |
| **B short press** | Output ON/OFF |
| **B long press (2s)** | Exit VAR mode → Return to 5V |
| **C** | +200mV (long-press repeat supported) |

### QC Capabilities Display

| Button | Function |
|--------|----------|
| **C** | EXIT (return to normal screen) |

## Voltage Range

- **Class A**: 5V / 9V / 12V fixed, VAR 3.6V-12.0V
- **Class B**: 5V / 9V / 12V / 20V fixed, VAR 3.6V-20.0V

## Calibration

Adjust the following parameters to match your actual measurements:

```cpp
float vScale = 7.66;    // VBUS detection scale factor
float vi_0A  = 2.44;    // Current detection voltage at 0A
float vi_2A  = 2.85;    // Current detection voltage at 2A
```

## Library Dependencies

- `M5Unified` - M5Stack display and button control
- `ESP32_QC3_CTL` - QuickCharge 3.0 control library (this project)

## Compilation & Upload

1. Open `M5Stack_QC3trigger.ino` in Arduino IDE
2. Select board: `M5Stack-Core-ESP32`
3. Compile and upload to M5Stack

## Notes

- Connect to QC3.0 compatible charger
- Connect load before changing voltage when output is ON
- 20V output is only available with Class B compatible chargers
- Current detection uses ACS712xLCTR-05B (±5A)

## License

Copyright (c) 2025 @tomorrow56

MIT License
