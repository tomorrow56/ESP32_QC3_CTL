/***************************************************
 * M5Stack QC3 Trigger
 * QuickCharge 3.0 Voltage Controller with M5Stack
 * Based on M5Stack_QC3_test & M5Stack_PDtrigger_PPS
 * Copyright(c) @tomorrow56 all rights reserved
 ****************************************************/

#include <M5Unified.h>
#include <ESP32_QC3_CTL.h>

/* M5Stack Basic Core Pin config */
#define DP_H      13
#define DP_L      16
#define DM_H      26
#define DM_L      17
#define VBUS_I    35
#define VI_I      36
#define VBUSEN_O   2

/* M5Stack Core2 Pin config */
/*
#define DP_H      19
#define DP_L      13
#define DM_H      26
#define DM_L      14
#define VBUS_I    35
#define VI_I      36
#define VBUSEN_O  32
*/

/**********
 * Calibration parameter
 * Changed to match actual measurements with your equipment
 **********/
float vScale = 7.66;    // actual VBUS / VBUS_I
float vi_0A  = 2.44;   // VI_I(V) @ 0A output
float vi_2A  = 2.85;   // VI_I(V) @ 2A output
float vi_0cal = 0.0;
float vbus_i_temp[20]; // for moving average

ESP32_QC3_CTL qc3(DP_H, DP_L, DM_H, DM_L, VBUS_I, VBUSEN_O);

// QC voltage mode index: 0=5V, 1=9V, 2=12V, 3=VAR
uint8_t QC_IDX = 0;
bool OE = false;
bool VAR_CONTROL = false;
bool QC_DECODE_MODE = false;
bool qcDecodeFirstDraw = true;

// VAR mode voltage tracking (mV)
uint16_t varVoltage = 5000;

uint32_t updateTime = 0;
const uint8_t UPDATE_INTERVAL = 100;

// Long-press repeat for VAR mode (A/C buttons)
const uint32_t HOLD_START_MS  = 1000;
const uint32_t HOLD_REPEAT_MS = 120;
uint32_t holdStartA  = 0;
uint32_t holdStartC  = 0;
uint32_t lastRepeatA = 0;
uint32_t lastRepeatC = 0;

// BtnB long-press for VAR mode enter/exit
const uint32_t VAR_HOLD_MS = 2000;
uint32_t holdStartB = 0;
bool btnBLongFired = false;

// QC mode labels (VAR is entered via BtnB long-press)
static const char* QC_LABELS[] = {"5V", "9V", "12V", "20V"};
static const uint8_t QC_MODES[] = {
  ESP32_QC3_CTL::QC_5V,
  ESP32_QC3_CTL::QC_9V,
  ESP32_QC3_CTL::QC_12V,
  ESP32_QC3_CTL::QC_20V
};
static const uint8_t QC_MODE_COUNT = 4U;

static void setMainTextColor() {
  if (OE) {
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  }
}

static void drawTitle(const String &title) {
  M5.Display.setTextSize(1);
  M5.Display.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Display.drawCentreString(title, 160, 2, 4);
}

static void drawSubTitle(const String &sub) {
  M5.Display.setTextSize(1);
  setMainTextColor();
  M5.Display.drawString(sub, 10, 33, 4);
}

static void drawText(const String &text, int xPos, int yPos) {
  M5.Display.setTextSize(1);
  setMainTextColor();
  M5.Display.drawString(text, xPos * 30, yPos * 30 + 60 + 3, 4);
}

static void drawBtnMenu(const String &a, const String &b, const String &c) {
  M5.Display.setTextSize(1);
  M5.Display.fillRect(0, 210, 320, 30, TFT_BLUE);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Display.drawCentreString(a, 65,  214, 4);
  M5.Display.drawCentreString(b, 160, 214, 4);
  M5.Display.drawCentreString(c, 255, 214, 4);
}

static void drawChargerStatus() {
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  uint8_t ht = qc3.getHostType();
  String label;
  switch (ht) {
    case ESP32_QC3_CTL::QC3:    label = "QC3.0"; break;
    case ESP32_QC3_CTL::BC_DCP: label = "DCP  "; break;
    default:                    label = "N/A  "; break;
  }
  M5.Display.drawString("CHG:" + label, 200, 33, 2);
}

static bool is20VEnabled() {
  return qc3.getUseClassB();
}

static void drawQCModeList() {
  M5.Display.fillRect(0, 175, 320, 30, TFT_BLACK);

  const int listStartX = 20;
  const int listWidth  = 280;
  const int itemWidth  = listWidth / (int)QC_MODE_COUNT;

  for (uint8_t i = 0U; i < QC_MODE_COUNT; i++) {
    bool isSelected = (QC_IDX == i);
    bool isEnabled  = (i < 3U) || is20VEnabled();
    int textX = listStartX + i * itemWidth + itemWidth / 2;
    int textY = 183;

    if (isSelected) {
      int boxW = 38;
      int boxH = 18;
      int maxW = itemWidth - 8;
      if (boxW > maxW) {
        boxW = maxW;
      }
      int boxX = textX - (boxW / 2);
      int boxY = textY - 2;
      M5.Display.fillRect(boxX, boxY, boxW, boxH, TFT_CYAN);
      M5.Display.setTextColor(TFT_BLACK);
    } else if (!isEnabled) {
      M5.Display.setTextColor(TFT_DARKGREY);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }
    M5.Display.setTextSize(1);
    M5.Display.drawCentreString(QC_LABELS[i], textX, textY, 2);
  }
}

static void updateBtnLabels() {
  if (QC_DECODE_MODE) {
    drawBtnMenu("", "", "EXIT");
    return;
  }
  if (VAR_CONTROL) {
    drawBtnMenu("DOWN", OE ? "OFF" : "ON", "UP");
    return;
  }
  if (QC_IDX == 0U) {
    drawBtnMenu("CAP", OE ? "OFF" : "ON", "UP");
    return;
  }
  bool canGoUp = (QC_IDX < (QC_MODE_COUNT - 1U))
               && ((QC_IDX < (QC_MODE_COUNT - 2U)) || is20VEnabled());
  if (QC_IDX == (QC_MODE_COUNT - 1U)) {
    drawBtnMenu("DOWN", OE ? "OFF" : "ON", "");
    return;
  }
  drawBtnMenu("DOWN", OE ? "OFF" : "ON", canGoUp ? "UP" : "");
}

static float averageVI() {
  float sum = 0.0;
  for (int i = 0; i < 20; i++) {
    sum += vbus_i_temp[i];
  }
  return sum / 20.0;
}

static float readVoltageRaw(uint16_t vread) {
  float vdc;
  if (vread < 5) {
    vdc = 0.0;
  } else if (vread <= 1084) {
    vdc = 0.11 + (0.89 / 1084.0) * vread;
  } else if (vread <= 2303) {
    vdc = 1.0 + (1.0 / (2303.0 - 1084.0)) * (vread - 1084);
  } else if (vread <= 3179) {
    vdc = 2.0 + (0.7 / (3179.0 - 2303.0)) * (vread - 2303);
  } else if (vread <= 3659) {
    vdc = 2.7 + (0.3 / (3659.0 - 3179.0)) * (vread - 3179);
  } else if (vread <= 4071) {
    vdc = 3.0 + (0.2 / (4071.0 - 3659.0)) * (vread - 3659);
  } else {
    vdc = 3.2;
  }
  return vdc;
}

static void applyQCMode() {
  if (VAR_CONTROL) {
    return;
  }
  (void)qc3.set_VBUS(QC_MODES[QC_IDX]);
}

static void enterVarControl() {
  VAR_CONTROL = true;
  (void)qc3.set_VBUS(ESP32_QC3_CTL::QC_VAR);
  varVoltage = qc3.getVoltage();
  updateBtnLabels();
}

static void drawQCCapabilities() {
  const int panelX = 20;
  const int panelY = 46;
  const int panelW = 280;
  const int panelH = 156;
  const int headerH = 24;

  if (qcDecodeFirstDraw) {
    M5.Display.fillScreen(TFT_BLACK);
    drawTitle("QC Capabilities");
    drawBtnMenu("", "", "EXIT");

    M5.Display.fillRect(panelX + 3, panelY + 3, panelW, panelH, TFT_DARKGREY);
    M5.Display.fillRect(panelX, panelY, panelW, panelH, TFT_BLACK);
    M5.Display.drawRect(panelX, panelY, panelW, panelH, TFT_CYAN);
    M5.Display.fillRect(panelX + 2, panelY + 2, panelW - 4, headerH, TFT_BLUE);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLUE);
    M5.Display.drawCentreString("Charger Capabilities", panelX + panelW / 2, panelY + 4, 2);

    qcDecodeFirstDraw = false;
  }

  M5.Display.fillRect(panelX + 4, panelY + headerH + 4, panelW - 8, panelH - headerH - 8, TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  uint8_t ht = qc3.getHostType();
  bool classB = qc3.getUseClassB();
  int lineY = panelY + headerH + 10;
  const int lineH = 18;

  // Charger type
  String typeStr;
  switch (ht) {
    case ESP32_QC3_CTL::QC3:    typeStr = "Type  : QC3.0"; break;
    case ESP32_QC3_CTL::BC_DCP: typeStr = "Type  : BC1.2 DCP"; break;
    default:                    typeStr = "Type  : Not detected"; break;
  }
  M5.Display.drawString(typeStr, panelX + 12, lineY, 2);
  lineY += lineH;

  if (ht == ESP32_QC3_CTL::QC3) {
    M5.Display.drawString("Class : " + String(classB ? "B (max 20V)" : "A (max 12V)"),
      panelX + 12, lineY, 2);
    lineY += lineH;

    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.drawString("Supported voltages:", panelX + 12, lineY, 2);
    lineY += lineH;
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

    M5.Display.drawString(" Fixed : 5V / 9V / 12V", panelX + 12, lineY, 2);
    lineY += lineH;

    if (classB) {
      M5.Display.drawString(" Fixed : 20V", panelX + 12, lineY, 2);
      lineY += lineH;
    }

    M5.Display.drawString(" VAR   : 3.6V - " + String(classB ? "20.0" : "12.0") + "V (200mV step)",
      panelX + 12, lineY, 2);
  } else {
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.drawString("QC3.0 not available.", panelX + 12, lineY, 2);
  }
}

static void exitVarControl() {
  VAR_CONTROL = false;
  QC_IDX = 0U;
  (void)qc3.set_VBUS(QC_MODES[QC_IDX]);
  varVoltage = qc3.getVoltage();
  drawQCModeList();
  updateBtnLabels();
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);

  M5.Display.fillScreen(TFT_BLACK);
  drawTitle("M5 QC3 Trigger");
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawCentreString("Detecting...", 160, 100, 4);

  // QC3 charger detection (blocks ~1.5s)
  qc3.begin();
  uint8_t ht = qc3.detect_Charger();

  // Initial voltage: 5V
  QC_IDX = 0U;
  VAR_CONTROL = false;
  (void)qc3.set_VBUS(QC_MODES[QC_IDX]);
  varVoltage = qc3.getVoltage();

  // Output disable
  digitalWrite(VBUSEN_O, LOW);
  OE = false;

  // Redraw UI
  M5.Display.fillScreen(TFT_BLACK);
  drawTitle("M5 QC3 Trigger");
  drawChargerStatus();
  drawQCModeList();
  updateBtnLabels();
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  // Moving average init
  for (int i = 0; i < 20; i++) {
    vbus_i_temp[i] = readVoltageRaw((uint16_t)analogRead(VI_I));
    delay(20);
  }
  vi_0cal = averageVI();

  updateTime = millis();

  Serial.begin(115200);
  Serial.print("Charger type: ");
  switch (ht) {
    case ESP32_QC3_CTL::QC3:    Serial.println("QC3.0"); break;
    case ESP32_QC3_CTL::BC_DCP: Serial.println("BC1.2 DCP"); break;
    default:                    Serial.println("N/A"); break;
  }
}

void loop() {
  M5.update();

  if (updateTime <= millis()) {
    updateTime = millis() + UPDATE_INTERVAL;

    if (QC_DECODE_MODE) {
      if (qcDecodeFirstDraw) {
        drawQCCapabilities();
      }
      return;
    }

    float vbusV = readVoltageRaw((uint16_t)analogRead(VBUS_I)) * vScale;

    for (int i = 19; i > 0; i--) {
      vbus_i_temp[i] = vbus_i_temp[i - 1];
    }
    vbus_i_temp[0] = readVoltageRaw((uint16_t)analogRead(VI_I));
    float vbusI = (averageVI() - vi_0cal) / ((vi_2A - vi_0A) / 2.0);

    char buf1[6];
    char buf2[6];
    dtostrf(vbusV, 4, 1, buf1);
    drawText("Vout = " + String(buf1) + " V    ", 1, 0);
    Serial.printf("Vout = %s V\n", buf1);

    dtostrf(vbusI, 5, 2, buf2);
    drawText("Iout = " + String(buf2) + " A    ", 1, 1);
    Serial.printf("Iout = %s A\n", buf2);

    // Subtitle: current mode
    if (VAR_CONTROL) {
      char varBuf[8];
      dtostrf((float)varVoltage / 1000.0, 4, 1, varBuf);
      drawSubTitle("VAR " + String(varBuf) + "V      ");
    } else {
      drawSubTitle("QC " + String(QC_LABELS[QC_IDX]) + "          ");
    }

    if (OE) {
      digitalWrite(VBUSEN_O, HIGH);
      Serial.println("Output Enabled");
    } else {
      digitalWrite(VBUSEN_O, LOW);
      Serial.println("Output Disabled");
      vi_0cal = averageVI();
    }
  }

  // --- Button A: DOWN / CAP / VAR -200mV ---
  if (M5.BtnA.wasPressed()) {
    if (QC_DECODE_MODE) {
      // nothing
    } else if (VAR_CONTROL) {
      // Decrease 200mV
      uint16_t varMin = qc3.getUseClassB() ? 3600U : 5000U;
      if (varVoltage > varMin + 200U) {
        qc3.var_dec();
        varVoltage = qc3.getVoltage();
      }
    } else if (QC_IDX == 0U) {
      // Enter QC Capabilities decode mode
      QC_DECODE_MODE = true;
      qcDecodeFirstDraw = true;
      updateBtnLabels();
    } else if (QC_IDX > 0U) {
      QC_IDX--;
      applyQCMode();
      drawQCModeList();
      updateBtnLabels();
    }
  }

  // --- Button B: short=Output ON/OFF, long(2s)=VAR enter/exit ---
  if (!QC_DECODE_MODE) {
    uint32_t now = millis();
    if (M5.BtnB.isPressed()) {
      if (holdStartB == 0U) {
        holdStartB    = now;
        btnBLongFired = false;
      } else if (!btnBLongFired && (now - holdStartB) >= VAR_HOLD_MS) {
        btnBLongFired = true;
        if (VAR_CONTROL) {
          exitVarControl();
        } else {
          enterVarControl();
        }
      }
    } else {
      if (holdStartB != 0U && !btnBLongFired) {
        // short press released
        OE = !OE;
        updateBtnLabels();
        setMainTextColor();
      }
      holdStartB    = 0U;
      btnBLongFired = false;
    }
  }

  // --- Button C: UP / VAR +200mV / EXIT decode ---
  if (M5.BtnC.wasPressed()) {
    if (QC_DECODE_MODE) {
      // Exit decode mode
      QC_DECODE_MODE = false;
      qcDecodeFirstDraw = true;
      M5.Display.fillScreen(TFT_BLACK);
      drawTitle("M5 QC3 Trigger");
      drawChargerStatus();
      drawQCModeList();
      updateBtnLabels();
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    } else if (VAR_CONTROL) {
      // Increase 200mV
      uint16_t varMax = qc3.getUseClassB() ? 20000U : 12000U;
      if (varVoltage < varMax) {
        qc3.var_inc();
        varVoltage = qc3.getVoltage();
      }
    } else if (QC_IDX < (QC_MODE_COUNT - 1U)) {
      uint8_t next = QC_IDX + 1U;
      if (next == (QC_MODE_COUNT - 1U) && !is20VEnabled()) {
        // 20V is disabled, skip
      } else {
        QC_IDX = next;
        applyQCMode();
        drawQCModeList();
        updateBtnLabels();
      }
    }
  }


  // --- Long-press repeat in VAR mode ---
  if (QC_DECODE_MODE) {
    return;
  }
  if (VAR_CONTROL) {
    uint32_t now = millis();
    uint16_t varMin = qc3.getUseClassB() ? 3600U : 5000U;
    uint16_t varMax = qc3.getUseClassB() ? 20000U : 12000U;

    if (M5.BtnA.isPressed()) {
      if (holdStartA == 0U) {
        holdStartA  = now;
        lastRepeatA = now;
      } else if ((now - holdStartA) >= HOLD_START_MS
               && (now - lastRepeatA) >= HOLD_REPEAT_MS) {
        if (varVoltage > varMin + 200U) {
          qc3.var_dec();
          varVoltage = qc3.getVoltage();
        }
        lastRepeatA = now;
      }
    } else {
      holdStartA  = 0U;
      lastRepeatA = 0U;
    }

    if (M5.BtnC.isPressed()) {
      if (holdStartC == 0U) {
        holdStartC  = now;
        lastRepeatC = now;
      } else if ((now - holdStartC) >= HOLD_START_MS
               && (now - lastRepeatC) >= HOLD_REPEAT_MS) {
        if (varVoltage < varMax) {
          qc3.var_inc();
          varVoltage = qc3.getVoltage();
        }
        lastRepeatC = now;
      }
    } else {
      holdStartC  = 0U;
      lastRepeatC = 0U;
    }
  } else {
    holdStartA  = 0U;
    holdStartC  = 0U;
    lastRepeatA = 0U;
    lastRepeatC = 0U;
  }
}
