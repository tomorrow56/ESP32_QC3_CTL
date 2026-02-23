# AtomS3_QC3_WebUI

M5Stack ATOM S3でQuickCharge 3.0出力を制御するWebUIサンプルです。
ATOM S3がアクセスポイント(AP)を起動し、ブラウザから出力電圧(5/9/12/20V, 可変±200mV)と出力ON/OFFを操作できます。

## 必要環境

- Arduino IDE
- ボード: ESP32S3系（ATOM S3）
- ライブラリ:
  - M5Unified
  - FastLED
  - このリポジトリのESP32_QC3_CTL

## 配線/ピン

`PinDefinitions.h` で定義しています。

- `DP_H`/`DP_L`/`DM_H`/`DM_L`: QC3制御（D+/D-）
- `VBUS_DET`: VBUS検出（ADC）
- `OUT_EN`: 出力ON/OFF
- `LED_DATA_PIN`: ATOM S3内蔵LED

## 使い方

1. `AtomS3_QC3_WebUI.ino` をArduino IDEで開く
2. ボード/ポートを選択して書き込み
3. 起動後、Wi-Fiで `ATOMS3_AP` に接続（パスワード: `01234567`）
4. ブラウザで `http://192.168.4.1/` を開く

## 操作

- **WebUIボタン**: 電圧設定、±200mV、ON/OFF
- **ATOM S3本体ボタン（BtnA）**: 出力ON/OFFトグル
  - WebUIは `/state` を定期取得して表示を同期します

## HTTP API

WebUIは以下のエンドポイントを利用します。

- `/` : WebUI（HTML）
- `/voltage?value=5|9|12|20` : 固定電圧に設定
- `/offset?value=200|-200` : 可変モード(QC_VAR)で±200mV
- `/toggle?state=on|off` : 出力ON/OFF
- `/state` : 現在の出力ON/OFF状態（`on`/`off`）
- `/current` : 出力電圧の実測値（mV）
- `/use_class_b` : Class B使用可否（`true`/`false`）

## 出力電圧（実測）の換算について

`/current` は `VBUS_DET` のADC電圧を読み取り、分圧比を掛けてVBUS(mV)を算出します。

- デフォルト補正係数: `7.67`（抵抗分割 100kΩ/15kΩ想定）
- 変更箇所: `WebUI.cpp` 内の `VBUS_DIVIDER_RATIO`

実機の分圧抵抗が異なる場合は、この係数を環境に合わせて調整してください。

## 起動直後のOUT_ENグリッチ

起動直後に `OUT_EN` が一瞬ONになる現象を軽減するため、`setup()` 冒頭で `INPUT_PULLDOWN` を経由してLOWへ固定してから出力化しています。
ブートローダ段階の挙動まで含めて完全に抑止したい場合は、`OUT_EN` にプルダウン抵抗を追加する等のハード対策も検討してください。

## ライセンス

Copyright (c) 2025 @tomorrow56

MIT License
