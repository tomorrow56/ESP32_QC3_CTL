
#include "WebUI.h"

// アクセスポイントのSSIDとパスワードを設定
const char* ssid = "ATOMS3_AP"; // アクセスポイント名
const char* password = "01234567"; // パスワード（8文字以上）

// グローバル変数の定義
AsyncWebServer server(80);
bool isOn = false;    // ON/OFF状態のフラグ
bool isQcVal = false; // 連続モードフラグ

void setupWebUI() {
  // アクセスポイントを開始
  WiFi.softAP(ssid, password);
  delay(100);
  IPAddress ip(192,168,4,1);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(ip, ip, subnet);
  Serial.println("Access Point Started");
  Serial.print("AP name: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // アクセスポイントのIPアドレスを表示

  // HTMLページを表示
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });

  // 電圧変更を処理
  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
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
    request->send(200, "text/plain", "OK");
  });

  // 連続モードの処理
  server.on("/offset", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      //連続モードへ切り替え
      qc3.set_VBUS(ESP32_QC3_CTL::QC_VAR);
      delay(100);
      //連続モードフラグをセット
      isQcVal = true;
      Serial.println("<Continuous mode>");

      String value = request->getParam("value")->value();
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
    request->send(200, "text/plain", "OK");
  });

  // ON/OFF切り替えを処理
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      isOn = (state == "on");
      if(isOn == true){
        digitalWrite(OUT_EN, HIGH);
      }else{
        digitalWrite(OUT_EN, LOW);
      }
      Serial.println("Output " + String(isOn ? "ON" : "OFF"));
    }
    request->send(200, "text/plain", "OK");
  });

  // 現在の電圧値を測定しUIに送信
  server.on("/current", HTTP_GET, [](AsyncWebServerRequest *request){
    String currentValue = (String)(int)(qc3.getVoltage()); // VBUS検出値
    Serial.println("Output: " + currentValue);
    request->send(200, "text/plain", currentValue);
  });

  // _use_class_bの値をUIに送信
  server.on("/use_class_b", HTTP_GET, [](AsyncWebServerRequest *request){
    String useClassBValue = qc3.getUseClassB() ? "true" : "false";
    request->send(200, "text/plain", useClassBValue);
  });

  // サーバーを開始
  server.begin();
}

