# M5Stack QC3 Trigger

QuickCharge 3.0 電圧コントローラー for M5Stack  
M5Stackのボタン操作でQC3.0対応充電器の出力電圧を制御します。

## 機能

- **固定電圧モード**: 5V / 9V / 12V / 20V（Class B対応時のみ）
- **可変電圧モード**: 200mVステップで電圧を調整
- **QC3.0検出**: 自動で充電器タイプを検出・表示
- **電圧・電流計測**: VBUS出力電圧と電流をリアルタイム表示
- **QC Capabilities表示**: 接続先充電器の対応電圧一覧を表示
- **出力ON/OFF制御**: VBUSENピンでFETゲート制御

## ハードウェア構成

### M5Stack Basic/Gray/Core ピン割り当て

| 信号 | ピン | 機能 |
|------|------|------|
| DP_H | 13 | D+ HIGH（10kΩ） |
| DP_L | 16 | D+ LOW（2.2kΩ） |
| DM_H | 26 | D- HIGH（10kΩ） |
| DM_L | 17 | D- LOW（2.2kΩ） |
| VBUS_I | 35 | VBUS電圧検出（ADC） |
| VI_I | 36 | 電流検出（ACS712xLCTR-05B） |
| VBUSEN_O | 2 | 出力イネーブル（FETゲート） |

### M5Stack Core2 ピン割り当て（コメントアウト済み）

| 信号 | ピン |
|------|------|
| DP_H | 19 |
| DP_L | 13 |
| DM_H | 26 |
| DM_L | 14 |
| VBUSEN_O | 32 |

## 操作方法

### 通常モード

| ボタン | 機能 |
|--------|------|
| **A** | CAP（QC Capabilities表示） / 電圧DOWN |
| **B短押し** | 出力 ON/OFF |
| **B長押し（2秒）** | VARモード入/退出 |
| **C** | 電圧UP |

### VARモード（可変電圧）

| ボタン | 機能 |
|--------|------|
| **A** | -200mV（長押しリピート対応） |
| **B短押し** | 出力 ON/OFF |
| **B長押し（2秒）** | VARモード終了 → 5Vに戻る |
| **C** | +200mV（長押しリピート対応） |

### QC Capabilities表示

| ボタン | 機能 |
|--------|------|
| **C** | EXIT（通常画面に戻る） |

## 電圧範囲

- **Class A**: 5V / 9V / 12V固定、VAR 3.6V-12.0V
- **Class B**: 5V / 9V / 12V / 20V固定、VAR 3.6V-20.0V

## キャリブレーション

実測に合わせて以下のパラメータを調整してください：

```cpp
float vScale = 7.66;    // VBUS検出スケール係数
float vi_0A  = 2.44;    // 電流検出 0A時の電圧
float vi_2A  = 2.85;    // 電流検出 2A時の電圧
```

## ライブラリ依存

- `M5Unified` - M5Stackディスプレイ・ボタン制御
- `ESP32_QC3_CTL` - QuickCharge 3.0制御ライブラリ（本プロジェクト）

## コンパイル・書き込み

1. Arduino IDEで `M5Stack_QC3trigger.ino` を開く
2. ボード: `M5Stack-Core-ESP32` を選択
3. コンパイルしてM5Stackに書き込み

## 注意事項

- QC3.0対応充電器に接続してください
- 出力ON時は負荷を接続してから電圧を変更してください
- 20V出力はClass B対応充電器でのみ利用可能です
- 電流検出にはACS712xLCTR-05B（±5A）を使用しています

## ライセンス

Copyright (c) 2025 @tomorrow56

MIT License
