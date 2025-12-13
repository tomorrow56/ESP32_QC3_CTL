/**
 * @file ESP32_QC3_CTL.h
 * @brief ESP32用QuickCharge 3.0制御ライブラリ
 * @author Manus AI (based on tomorrow56's code)
 * @date 2025/06/02
 * 
 * このライブラリはESP32を使用してQuickCharge 3.0対応充電器の出力電圧を制御します。
 * 元のコード: https://github.com/tomorrow56/ESP32_QC3
 */

#ifndef ESP32_QC3_CTL_H
#define ESP32_QC3_CTL_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
 #if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR < 5)
  #include <driver/adc.h>
 #endif
#endif

/**
 * @brief QuickCharge 3.0制御クラス
 */
class ESP32_QC3_CTL {
public:
    /**
     * @brief D+/D-の設定状態
     */
    enum QC_STATE {
        QC_HIZ = 0x00,    ///< ハイインピーダンス状態
        QC_0V = 0x01,     ///< 0V
        QC_600mV = 0x02,  ///< 600mV
        QC_3300mV = 0x03  ///< 3300mV
    };

    /**
     * @brief ホストポートの種類
     */
    enum HOST_PORT_TYPE {
        BC_NA = 0x00,     ///< 非対応
        BC_DCP = 0x01,    ///< 通常の充電器
        QC3 = 0x02        ///< QC3.0対応充電器
    };

    /**
     * @brief VBUS出力設定値
     */
    enum QC_VOLTAGE_MODE {
        QC_5V = 0x00,     ///< 5V出力
        QC_9V = 0x01,     ///< 9V出力
        QC_12V = 0x02,    ///< 12V出力
        QC_20V = 0x03,    ///< 20V出力
        QC_VAR = 0x04     ///< 可変出力
    };

    /**
     * @brief コンストラクタ
     * @param dp_h D+端子のHIGHピン
     * @param dp_l D+端子のLOWピン
     * @param dm_h D-端子のHIGHピン
     * @param dm_l D-端子のLOWピン
     * @param vbus_det VBUS検出ピン
     * @param out_en 出力有効ピン（オプション）
     */
    ESP32_QC3_CTL(uint8_t dp_h, uint8_t dp_l, uint8_t dm_h, uint8_t dm_l, uint8_t vbus_det, uint8_t out_en = 0);

    /**
     * @brief 初期化
     * @return 初期化結果（true: 成功, false: 失敗）
     */
    bool begin();

    /**
     * @brief ADCの検出値を電圧値に変換する
     * @param Vread ADCの読み取り値
     * @return 変換後の電圧値（V）
     * @note ESP32のADCの直線性が悪い部分は折線で近似
     */
    float readVoltage(uint16_t Vread);

    float readVoltage(uint8_t pin, uint16_t Vread);

    float readVoltage(uint8_t pin);

    bool addAdcPin(uint8_t pin);

    bool addAdcPin(uint8_t pin, uint8_t attenuation);

    /**
     * @brief D+端子への印加電圧設定
     * @param state 設定状態（QC_HIZ, QC_0V, QC_600mV, QC_3300mV）
     */
    void set_DP(uint8_t state);

    /**
     * @brief D-端子への印加電圧設定
     * @param state 設定状態（QC_HIZ, QC_0V, QC_600mV, QC_3300mV）
     */
    void set_DM(uint8_t state);

    /**
     * @brief VBUS出力電圧設定
     * @param mode 電圧モード（QC_5V, QC_9V, QC_12V, QC_20V, QC_VAR）
     * @return 設定結果（true: 成功, false: 失敗）
     */
    bool set_VBUS(uint8_t mode);

    /**
     * @brief 連続動作モード - 電圧増加
     * @note QC_VARモードでのみ有効、200mVずつ増加
     */
    void var_inc();

    /**
     * @brief 連続動作モード - 電圧減少
     * @note QC_VARモードでのみ有効、200mVずつ減少
     */
    void var_dec();

    /**
     * @brief 接続されたポートの検出
     * @return ポートタイプ（BC_NA, BC_DCP, QC3）
     */
    uint8_t detect_Charger();

    /**
     * @brief 現在の出力電圧値を取得
     * @return 出力電圧値（mV）
     */
    uint16_t getVoltage();

    /**
     * @brief 現在のホストタイプを取得
     * @return ホストタイプ（BC_NA, BC_DCP, QC3）
     */
    uint8_t getHostType();

    /**
     * @brief _use_class_bの現在の値を取得
     * @return _use_class_bの値
     */
    bool getUseClassB();

private:
    static const uint8_t MAX_ADC_PINS = 8U;

    uint8_t _adcPins[MAX_ADC_PINS];
    uint8_t _adcPinAtten[MAX_ADC_PINS];
    uint8_t _adcPinCount;

    uint8_t _dp_h;        ///< D+端子のHIGHピン
    uint8_t _dp_l;        ///< D+端子のLOWピン
    uint8_t _dm_h;        ///< D-端子のHIGHピン
    uint8_t _dm_l;        ///< D-端子のLOWピン
    uint8_t _vbus_det;    ///< VBUS検出ピン
    uint8_t _out_en;      ///< 出力有効ピン

    uint16_t _dp_val;     ///< D+端子の電圧値
    uint16_t _dm_val;     ///< D-端子の電圧値
    uint16_t _vbus_det_val; ///< VBUS検出値

    uint8_t _host_type;   ///< ホストポートの種類
    uint16_t _vbus_val;   ///< VBUS出力設定値
    uint8_t _qc_mode;     ///< QCモード

    bool _is_on;          ///< 出力ON/OFF状態

    // VBUS可変範囲
    static const uint16_t QC3_VAR_MIN = 5000;  ///< 最小電圧（mV）
    static const uint16_t QC3A_VAR_MAX = 12000; ///< 最大電圧 Class A（mV）
    static const uint16_t QC3B_VAR_MAX = 20000; ///< 最大電圧 Class B（mV）
    
    bool _use_class_b;    ///< Class B使用フラグ
};

#endif // ESP32_QC3_CTL_H
