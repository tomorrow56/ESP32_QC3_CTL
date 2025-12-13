/**
 * @file ESP32_QC3_CTL.cpp
 * @brief ESP32用QuickCharge 3.0制御ライブラリの実装
 * @author Manus AI (based on tomorrow56's code)
 * @date 2025/06/02
 * 
 * このライブラリはESP32を使用してQuickCharge 3.0対応充電器の出力電圧を制御します。
 * 元のコード: https://github.com/tomorrow56/ESP32_QC3
 */

#include "ESP32_QC3_CTL.h"

#if defined(ARDUINO_ARCH_ESP32)
 #if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)
  #include <driver/adc.h>
  #include <esp_adc_cal.h>
  #if defined(__has_include)
   #if __has_include(<esp_adc/adc_oneshot.h>)
    #include <esp_adc/adc_oneshot.h>
   #endif
  #endif
 #endif
 #include <esp_err.h>
#endif

#if defined(ARDUINO_ARCH_ESP32) && defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)
static bool getAdcUnitFromPin(uint8_t pin, adc_unit_t *unit) {
#if defined(ESP_IDF_VERSION_MAJOR) && defined(__has_include)
 #if __has_include(<esp_adc/adc_oneshot.h>)
    adc_channel_t channel;
    if (adc_oneshot_io_to_channel((gpio_num_t)pin, unit, &channel) == ESP_OK) {
        return true;
    }
 #endif
#endif

#if defined(CONFIG_IDF_TARGET_ESP32)
    switch (pin) {
        case 36:
        case 37:
        case 38:
        case 39:
        case 32:
        case 33:
        case 34:
        case 35:
            *unit = ADC_UNIT_1;
            return true;
        case 4:
        case 0:
        case 2:
        case 15:
        case 13:
        case 12:
        case 14:
        case 27:
        case 25:
        case 26:
            *unit = ADC_UNIT_2;
            return true;
        default:
            return false;
    }
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    if ((pin >= 1U) && (pin <= 10U)) {
        *unit = ADC_UNIT_1;
        return true;
    }
    if ((pin >= 11U) && (pin <= 20U)) {
        *unit = ADC_UNIT_2;
        return true;
    }
    return false;
#else
    (void)pin;
    return false;
#endif
}
#endif

/**
 * @brief コンストラクタ
 * @param dp_h D+端子のHIGHピン
 * @param dp_l D+端子のLOWピン
 * @param dm_h D-端子のHIGHピン
 * @param dm_l D-端子のLOWピン
 * @param vbus_det VBUS検出ピン
 * @param out_en 出力有効ピン（オプション）
 */
ESP32_QC3_CTL::ESP32_QC3_CTL(uint8_t dp_h, uint8_t dp_l, uint8_t dm_h, uint8_t dm_l, uint8_t vbus_det, uint8_t out_en) {
    _dp_h = dp_h;
    _dp_l = dp_l;
    _dm_h = dm_h;
    _dm_l = dm_l;
    _vbus_det = vbus_det;
    _out_en = out_en;
    
    _dp_val = 0;
    _dm_val = 0;
    _vbus_det_val = 0;
    
    _host_type = BC_NA;
    _vbus_val = 0;
    _qc_mode = QC_5V;
    
    _is_on = false;
    _use_class_b = false;

    _adcPinCount = 0U;
    for (uint8_t i = 0U; i < MAX_ADC_PINS; i++) {
        _adcPins[i] = 0U;
        _adcPinAtten[i] = 0U;
    }
}

/**
 * @brief 初期化
 * @return 初期化結果（true: 成功, false: 失敗）
 */
bool ESP32_QC3_CTL::begin() {
    // ピンの初期化
    pinMode(_vbus_det, INPUT);

#if defined(ARDUINO_ARCH_ESP32)
    analogReadResolution(12);
    (void)addAdcPin(_dp_h);
    (void)addAdcPin(_dm_h);
    (void)addAdcPin(_vbus_det);
    for (uint8_t i = 0U; i < _adcPinCount; i++) {
        analogSetPinAttenuation(
            _adcPins[i],
            (adc_attenuation_t)_adcPinAtten[i]
        );
    }
#endif
    
    if (_out_en > 0) {
        pinMode(_out_en, OUTPUT);
        digitalWrite(_out_en, LOW);
    }
    
    // D+/D-をハイインピーダンス状態に設定
    set_DP(QC_HIZ);
    set_DM(QC_HIZ);
    
    return true;
}

/**
 * @brief ADCの検出値を電圧値に変換する
 * @param Vread ADCの読み取り値
 * @return 変換後の電圧値（V）
 * @note ESP32のADCの直線性が悪い部分は折線で近似
 */
float ESP32_QC3_CTL::readVoltage(uint16_t Vread) {
    float Vdc;

#if defined(ARDUINO_ARCH_ESP32)
 #if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)
    static bool isCalibrated = false;
    static esp_adc_cal_characteristics_t adcChars;
    if (!isCalibrated) {
        (void)esp_adc_cal_characterize(
            ADC_UNIT_1,
            ADC_ATTEN_DB_11,
            ADC_WIDTH_BIT_12,
            0,
            &adcChars
        );
        isCalibrated = true;
    }
    Vdc = (float)esp_adc_cal_raw_to_voltage(Vread, &adcChars) / 1000.0f;
 #else
    Vdc = ((float)Vread / 4095.0f) * 3.3f;
 #endif
#else
    Vdc = 0.03f + ((float)Vread / 4096.0f) * 3.3f;
#endif
    
    /* ESP32用の変換式（コメントアウト）
    if(Vread < 5) {
        Vdc = 0;
    } else if(Vread <= 1084) {
        Vdc = 0.11 + (0.89 / 1084) * Vread;
    } else if(Vread <= 2303) {
        Vdc = 1.0 + (1.0 / (2303 - 1084)) * (Vread - 1084);
    } else if(Vread <= 3179) {
        Vdc = 2.0 + (0.7 / (3179 - 2303)) * (Vread - 2303);
    } else if(Vread <= 3659) {
        Vdc = 2.7 + (0.3 / (3659 - 3179)) * (Vread - 3179);
    } else if(Vread <= 4071) {
        Vdc = 3.0 + (0.2 / (4071 - 3659)) * (Vread - 3659);
    } else {
        Vdc = 3.2;
    }
    */
    
    return Vdc;
}

float ESP32_QC3_CTL::readVoltage(uint8_t pin, uint16_t Vread) {
    float Vdc;

#if defined(ARDUINO_ARCH_ESP32)
 #if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)
    adc_unit_t unit = ADC_UNIT_1;
    const bool hasUnit = getAdcUnitFromPin(pin, &unit);
    if (hasUnit) {
        static bool isCalibratedUnit1 = false;
        static bool isCalibratedUnit2 = false;
        static esp_adc_cal_characteristics_t adcCharsUnit1;
        static esp_adc_cal_characteristics_t adcCharsUnit2;

        if ((unit == ADC_UNIT_2) && (!isCalibratedUnit2)) {
            (void)esp_adc_cal_characterize(
                ADC_UNIT_2,
                ADC_ATTEN_DB_11,
                ADC_WIDTH_BIT_12,
                0,
                &adcCharsUnit2
            );
            isCalibratedUnit2 = true;
        }

        if ((unit != ADC_UNIT_2) && (!isCalibratedUnit1)) {
            (void)esp_adc_cal_characterize(
                ADC_UNIT_1,
                ADC_ATTEN_DB_11,
                ADC_WIDTH_BIT_12,
                0,
                &adcCharsUnit1
            );
            isCalibratedUnit1 = true;
        }

        if ((unit == ADC_UNIT_2) && isCalibratedUnit2) {
            Vdc = (float)esp_adc_cal_raw_to_voltage(Vread, &adcCharsUnit2) /
                1000.0f;
            return Vdc;
        }
        if ((unit != ADC_UNIT_2) && isCalibratedUnit1) {
            Vdc = (float)esp_adc_cal_raw_to_voltage(Vread, &adcCharsUnit1) /
                1000.0f;
            return Vdc;
        }
    }

    Vdc = readVoltage(Vread);
 #else
    (void)pin;
    Vdc = readVoltage(Vread);
 #endif
#else
    (void)pin;
    Vdc = readVoltage(Vread);
#endif

    return Vdc;
}

float ESP32_QC3_CTL::readVoltage(uint8_t pin) {
    return readVoltage(pin, (uint16_t)analogRead(pin));
}

bool ESP32_QC3_CTL::addAdcPin(uint8_t pin) {
#if defined(ARDUINO_ARCH_ESP32)
    return addAdcPin(pin, (uint8_t)ADC_11db);
#else
    return addAdcPin(pin, 0U);
#endif
}

bool ESP32_QC3_CTL::addAdcPin(uint8_t pin, uint8_t attenuation) {
    for (uint8_t i = 0U; i < _adcPinCount; i++) {
        if (_adcPins[i] == pin) {
            _adcPinAtten[i] = attenuation;
            return true;
        }
    }

    if (_adcPinCount >= MAX_ADC_PINS) {
        return false;
    }

    _adcPins[_adcPinCount] = pin;
    _adcPinAtten[_adcPinCount] = attenuation;
    _adcPinCount++;
    return true;
}

/**
 * @brief D+端子への印加電圧設定
 * @param state 設定状態（QC_HIZ, QC_0V, QC_600mV, QC_3300mV）
 */
void ESP32_QC3_CTL::set_DP(uint8_t state) {
    if(state == QC_HIZ) {
        pinMode(_dp_h, INPUT);
        pinMode(_dp_l, INPUT);
    } else {
        pinMode(_dp_h, OUTPUT);
        pinMode(_dp_l, OUTPUT);
        
        if(state == QC_0V) {
            digitalWrite(_dp_h, LOW);
            digitalWrite(_dp_l, LOW);
        } else if(state == QC_600mV) {
            digitalWrite(_dp_h, HIGH);
            digitalWrite(_dp_l, LOW);
        } else if(state == QC_3300mV) {
            digitalWrite(_dp_h, HIGH);
            digitalWrite(_dp_l, HIGH);
        } else {
            digitalWrite(_dp_h, LOW);
            digitalWrite(_dp_l, LOW);
        }
    }
}

/**
 * @brief D-端子への印加電圧設定
 * @param state 設定状態（QC_HIZ, QC_0V, QC_600mV, QC_3300mV）
 */
void ESP32_QC3_CTL::set_DM(uint8_t state) {
    if(state == QC_HIZ) {
        pinMode(_dm_h, INPUT);
        pinMode(_dm_l, INPUT);
    } else {
        pinMode(_dm_h, OUTPUT);
        pinMode(_dm_l, OUTPUT);
        
        if(state == QC_0V) {
            digitalWrite(_dm_h, LOW);
            digitalWrite(_dm_l, LOW);
        } else if(state == QC_600mV) {
            digitalWrite(_dm_h, HIGH);
            digitalWrite(_dm_l, LOW);
        } else if(state == QC_3300mV) {
            digitalWrite(_dm_h, HIGH);
            digitalWrite(_dm_l, HIGH);
        } else {
            digitalWrite(_dm_h, LOW);
            digitalWrite(_dm_l, LOW);
        }
    }
}

/**
 * @brief VBUS出力電圧設定
 * @param mode 電圧モード（QC_5V, QC_9V, QC_12V, QC_20V, QC_VAR）
 * @return 設定結果（true: 成功, false: 失敗）
 */
bool ESP32_QC3_CTL::set_VBUS(uint8_t mode) {
    if(_host_type != QC3) {
        return false;
    }
    
    _qc_mode = mode;
    
    switch(mode) {
        case QC_5V:
            set_DP(QC_600mV);
            set_DM(QC_0V);
            _vbus_val = 5000;
            break;
        case QC_9V:
            set_DP(QC_3300mV);
            set_DM(QC_600mV);
            _vbus_val = 9000;
            break;
        case QC_12V:
            set_DP(QC_600mV);
            set_DM(QC_600mV);
            _vbus_val = 12000;
            break;
        case QC_20V:
            set_DP(QC_3300mV);
            set_DM(QC_3300mV);
            _vbus_val = 20000;
            break;
        case QC_VAR:
            set_DP(QC_600mV);
            set_DM(QC_3300mV);
            break;
        default:
            set_DP(QC_600mV);
            set_DM(QC_0V);
            _vbus_val = 5000;
            break;
    }
    
    return true;
}

/**
 * @brief 連続動作モード - 電圧増加
 * @note QC_VARモードでのみ有効、200mVずつ増加
 */
void ESP32_QC3_CTL::var_inc() {
    if(_qc_mode != QC_VAR) {
        return;
    }
    
    uint16_t QC3_VAR_MAX;
    if(_use_class_b) {
        QC3_VAR_MAX = QC3B_VAR_MAX;
    } else {
        QC3_VAR_MAX = QC3A_VAR_MAX;
    }
    
    _vbus_val = _vbus_val + 200;
    if(_vbus_val > QC3_VAR_MAX) {
        _vbus_val = QC3_VAR_MAX;
    } else {
        set_DP(QC_3300mV);
        delayMicroseconds(200);
        set_DP(QC_600mV);
        delay(100);
    }
}

/**
 * @brief 連続動作モード - 電圧減少
 * @note QC_VARモードでのみ有効、200mVずつ減少
 */
void ESP32_QC3_CTL::var_dec() {
    if(_qc_mode != QC_VAR) {
        return;
    }
    
    _vbus_val = _vbus_val - 200;
    if(_vbus_val < QC3_VAR_MIN) {
        _vbus_val = QC3_VAR_MIN;
    } else {
        set_DM(QC_600mV);
        delayMicroseconds(200);
        set_DM(QC_3300mV);
        delay(100);
    }
}

/**
 * @brief 接続されたポートの検出
 * @return ポートタイプ（BC_NA, BC_DCP, QC3）
 */
uint8_t ESP32_QC3_CTL::detect_Charger() {
    set_DP(QC_HIZ);
    set_DM(QC_HIZ);
    
    // stage 1: check BC1.2 DCP
    set_DM(QC_0V);
    
    // ADC to Voltage(mV)
    _dp_val = readVoltage(_dp_h) * 1000;
    
    if(_dp_val >= 325) {
        set_DM(QC_HIZ);
        _host_type = BC_NA;
        return BC_NA;
    } else {
        // stage 2: set host to QC3
        set_DM(QC_HIZ);
        set_DP(QC_600mV);
        delay(1500);
        
        // ADC to Voltage(mV)
        _dm_val = readVoltage(_dm_h) * 1000;
        
        if(_dm_val >= 325) {
            set_DP(QC_HIZ);
            _host_type = BC_DCP;
            return BC_DCP;
        } else {
            _host_type = QC3;
            // QC3.0検出後、20V設定時の電圧をチェックして_use_class_bを設定
            set_VBUS(QC_20V);
            delay(100);
            float detected_voltage = readVoltage(_vbus_det);
            if (detected_voltage >= 19.0f) {
                _use_class_b = true;
            } else {
                _use_class_b = false;
            }
            set_VBUS(QC_5V); // 初期状態に戻す
            return QC3;
        }
    }
}

/**
 * @brief 現在の出力電圧値を取得
 * @return 出力電圧値（mV）
 */
uint16_t ESP32_QC3_CTL::getVoltage() {
    return _vbus_val;
}

/**
 * @brief 現在のホストタイプを取得
 * @return ホストタイプ（BC_NA, BC_DCP, QC3）
 */
uint8_t ESP32_QC3_CTL::getHostType() {
    return _host_type;
}


/**
 * @brief _use_class_bの現在の値を取得
 * @return _use_class_bの値
 */
bool ESP32_QC3_CTL::getUseClassB() {
    return _use_class_b;
}
