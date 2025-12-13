/********************
 * M5Stack Include
 ********************/
#include <M5Unified.h>
#include <FastLED.h>

/********************
 * Pin Definitions
 ********************/
#include "PinDefinitions.h"

/********************
 * QC3 Library Include
 ********************/
#include <ESP32_QC3_CTL.h>

/********************
 * WebUI Include
 ********************/
#include "WebUI.h"

/********************
 * LED設定
 ********************/
CRGB leds[NUM_LEDS];

// ESP32_QC3ライブラリのインスタンスを作成
ESP32_QC3_CTL qc3(DP_H, DP_L, DM_H, DM_L, VBUS_DET, OUT_EN);

// グローバル変数の宣言（WebUI.hでexternとして宣言済み）

void setup() {
  pinMode(OUT_EN, INPUT_PULLDOWN);
  digitalWrite(OUT_EN, LOW);
  pinMode(OUT_EN, OUTPUT);

  auto cfg = M5.config();
  M5.begin(cfg);
  // シリアルモニタを開始
  Serial.begin(115200);

  //FastLED初期設定
  FastLED.addLeds<WS2812, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(200);

  // QC3ライブラリの初期化
  qc3.begin();

  // Chargerの種類を検出する
  uint8_t HOST_TYPE = qc3.detect_Charger();

  Serial.print("Charger type: ");

  switch(HOST_TYPE){
  case ESP32_QC3_CTL::BC_NA:
    Serial.println("No charging port");
    // NAの時はLEDを黄色にする
    leds[0] = CRGB::Yellow;
    FastLED.show();
    delay(10);
    break;
  case ESP32_QC3_CTL::BC_DCP:
    Serial.println("USB BC1.2 DCP");
    // BC_DCPの時はLEDを橙色にする
    leds[0] = CRGB::Orange;
    FastLED.show();
    delay(10);
    break;
  case ESP32_QC3_CTL::QC3:
    Serial.println("QC3.0");
    // QC3の時はLEDを赤にする
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(10);
    // 初期値は5Vに設定
    qc3.set_VBUS(ESP32_QC3_CTL::QC_5V);
    delay(100);
    break;
  default:
    Serial.println("Unknown");
    // 不明の時はLEDを黄色にする
    leds[0] = CRGB::Yellow;
    FastLED.show();
    delay(10);
    break;
  }

  // WebUIのセットアップ
  setupWebUI();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    isOn = !isOn;
    digitalWrite(OUT_EN, isOn ? HIGH : LOW);
    Serial.println("Button toggle: " + String(isOn ? "ON" : "OFF"));
  }

  server.handleClient();

  // ON/OFF状態に応じてLEDを制御
  if(isOn == true){
    leds[0] = CRGB::Green;
  }else{
    leds[0] = CRGB::Red;
  }
  FastLED.show();
  delay(10);
}
