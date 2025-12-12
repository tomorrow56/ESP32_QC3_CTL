#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H

// ATOM S3 ピン定義
#define DP_H     5   // GPIO5, ADC1_CH4, 10k
#define DP_L     6   // GPIO6, 2.2k
#define DM_H     7   // GPIO7, ADC1_CH6, 10k
#define DM_L    39   // GPIO39, 2.2k
#define VBUS_DET 8   // GPIO8, ADC1_CH7
#define OUT_EN  38   // 出力有効化ピン

// LED設定
#define NUM_LEDS 1
#define LED_DATA_PIN 35

#endif // PINDEFINITIONS_H
