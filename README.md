# ESP32_QC3_CTL

ESP32用のQuickCharge 3.0制御ライブラリです。

## インストール

- Arduino IDEのスケッチブックフォルダ配下の`libraries`に、この`ESP32_QC3_CTL`フォルダを配置してください。

## 使い方

- `#include <ESP32_QC3_CTL.h>`
- `ESP32_QC3_CTL qc3(dp_h, dp_l, dm_h, dm_l, vbus_det, out_en);`
- `qc3.begin();`

`addAdcPin(pin, attenuation)`で任意のADC pinを登録すると、`begin()`でatten設定を自動適用します。

## examples

- `examples/DetectCharger/DetectCharger.ino`
- `examples/AtomS3_QC3_WebUI/AtomS3_QC3_WebUI.ino`
