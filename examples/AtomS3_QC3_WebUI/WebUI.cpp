 
#include "WebUI.h"

// アクセスポイントのSSIDとパスワードを設定
const char* ssid = "ATOMS3_AP"; // アクセスポイント名
const char* password = "01234567"; // パスワード（8文字以上）

// グローバル変数の定義
WebServer server(80);
bool isOn = false;    // ON/OFF状態のフラグ
bool isQcVal = false; // 連続モードフラグ

void setupWebUI() {
  // アクセスポイントを開始
  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  (void)WiFi.softAPConfig(ip, ip, subnet);
  const bool apOk = WiFi.softAP(ssid, password);
  delay(100);

  Serial.println("Access Point Started");
  Serial.print("AP start: ");
  Serial.println(apOk ? "OK" : "NG");
  Serial.print("AP name: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // アクセスポイントのIPアドレスを表示

  // HTMLページを表示
  server.on("/", HTTP_GET, [](){
    Serial.println("HTTP GET / ");
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send(200, "text/html", htmlPage);
  });

  // 電圧変更を処理
  server.on("/voltage", HTTP_GET, [](){
    Serial.println("HTTP GET /voltage");
    if (server.hasArg("value")) {
      const String value = server.arg("value");
      Serial.print("  value=");
      Serial.println(value);
      // 連続モードの場合は一旦5Vに設定
      if(isQcVal == true){
        qc3.set_VBUS(ESP32_QC3_CTL::QC_5V);
        isQcVal = false;
        delay(100);
      }

      // 出力電圧設定
      if(value == "5"){
        qc3.set_VBUS(ESP32_QC3_CTL::QC_5V);
      }else if(value == "9"){
        qc3.set_VBUS(ESP32_QC3_CTL::QC_9V);
      }else if(value == "12"){
        qc3.set_VBUS(ESP32_QC3_CTL::QC_12V);
      }else if(value == "20"){
        qc3.set_VBUS(ESP32_QC3_CTL::QC_20V);
      }
      Serial.print("Voltage set to: ");
      Serial.print(qc3.getVoltage());
      Serial.println("mV");
      delay(100);
    }
    server.send(200, "text/plain", "OK");
  });

  // 連続モードの処理
  server.on("/offset", HTTP_GET, [](){
    Serial.println("HTTP GET /offset");
    if (server.hasArg("value")) {
      //連続モードへ切り替え
      qc3.set_VBUS(ESP32_QC3_CTL::QC_VAR);
      delay(100);
      //連続モードフラグをセット
      isQcVal = true;
      Serial.println("<Continuous mode>");

      const String value = server.arg("value");
      Serial.print("  value=");
      Serial.println(value);
      if(value == "200"){
        qc3.var_inc();
      }else if(value == "-200"){
        qc3.var_dec();
      }
      Serial.print("Offset: " + value + " mV");
      Serial.print("Voltage set to: ");
      Serial.print(qc3.getVoltage());
      Serial.println("mV");
      delay(100);
    }
    server.send(200, "text/plain", "OK");
  });

  // ON/OFF切り替えを処理
  server.on("/toggle", HTTP_GET, [](){
    Serial.println("HTTP GET /toggle");
    if (server.hasArg("state")) {
      const String state = server.arg("state");
      Serial.print("  state=");
      Serial.println(state);
      isOn = (state == "on");
      if(isOn == true){
        digitalWrite(OUT_EN, HIGH);
      }else{
        digitalWrite(OUT_EN, LOW);
      }
      Serial.println("Output " + String(isOn ? "ON" : "OFF"));
    }
    server.send(200, "text/plain", "OK");
  });

  // 現在のON/OFF状態をUIに送信
  server.on("/state", HTTP_GET, [](){
    Serial.println("HTTP GET /state");
    server.send(200, "text/plain", isOn ? "on" : "off");
  });

  // 現在の電圧値を測定しUIに送信
  server.on("/current", HTTP_GET, [](){
    Serial.println("HTTP GET /current");
    const float vbusDetV = qc3.readVoltage((uint8_t)VBUS_DET);
    const float vbusDetMv = vbusDetV * 1000.0f;
    // 抵抗分割: 100kΩ / 15kΩ
    const float VBUS_DIVIDER_RATIO = 7.67f;
    const uint32_t vbusMv = (uint32_t)(vbusDetMv * VBUS_DIVIDER_RATIO);
    const String currentValue = String((uint32_t)vbusMv);
    Serial.println("Output: " + currentValue);
    server.send(200, "text/plain", currentValue);
  });

  // _use_class_bの値をUIに送信
  server.on("/use_class_b", HTTP_GET, [](){
    Serial.println("HTTP GET /use_class_b");
    const String useClassBValue = qc3.getUseClassB() ? "true" : "false";
    server.send(200, "text/plain", useClassBValue);
  });

  server.onNotFound([](){
    Serial.print("HTTP 404 ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "Not Found");
  });

  // サーバーを開始
  server.begin();
}
 
