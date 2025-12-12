#include <Arduino.h>
#include <ESP32_QC3_CTL.h>

// Set these pins for your board and wiring.
static const uint8_t DP_H = 5;
static const uint8_t DP_L = 6;
static const uint8_t DM_H = 7;
static const uint8_t DM_L = 39;
static const uint8_t VBUS_DET = 8;
static const uint8_t OUT_EN = 38;

ESP32_QC3_CTL qc3(DP_H, DP_L, DM_H, DM_L, VBUS_DET, OUT_EN);

void setup() {
  Serial.begin(115200);

  // Optional: register extra ADC pins and attenuation before begin().
  // qc3.addAdcPin(VBUS_DET, ADC_11db);

  qc3.begin();

  const uint8_t hostType = qc3.detect_Charger();
  Serial.print("Charger type: ");
  Serial.println(hostType);

  if (hostType == ESP32_QC3_CTL::QC3) {
    qc3.set_VBUS(ESP32_QC3_CTL::QC_5V);
    delay(100);
  }
}

void loop() {
  delay(1000);
}
