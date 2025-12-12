#ifndef WEBUI_H
#define WEBUI_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32_QC3_CTL.h>
#include "PinDefinitions.h"

// グローバル変数の宣言
extern AsyncWebServer server;
extern ESP32_QC3_CTL qc3;
extern bool isOn;      // ON/OFF状態のフラグ
extern bool isQcVal;   // 連続モードフラグ

// HTMLページの定義
const char* const htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>QuickCharge 3.0 Control</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; font-size: 24px;}
        .button { padding: 20px 40px; margin: 10px; font-size: 48px; border: 1px solid #ccc; border-radius: 5px; cursor: pointer; background-color: #fffdd0;}
        .button:hover { background-color: #fffdd0; }
        .on-off { background-color: red; color: white; } /* 初期状態は緑 */
        .voltage-label { font-size: 48px; }
        .voltage-value { font-size: 48px; color: blue; font-weight: bold;}
    </style>
</head>
<body>
    <h1>QuickCharge 3.0 Control</h1>
    <div>
        <label class=\"voltage-label\">Output: </label>
        <span id=\"current\" class=\"voltage-value\">0</span>
        <label class=\"voltage-label\"> mV</label>
    </div>
    <br>
    <div>
        <button class=\"button\" onclick=\"sendVoltage(5)\">5 V</button>
        <button class=\"button\" onclick=\"sendVoltage(9)\">9 V</button>
        <button class=\"button\" onclick=\"sendVoltage(12)\">12 V</button>
        <button class=\"button\" onclick=\"sendVoltage(20)\" id=\"button20V\">20 V</button>
    </div>
    <div>
        <button class=\"button\" onclick=\"sendOffset(-200)\">-200 mV</button>
        <button id=\"toggle-btn\" class=\"button on-off\" onclick=\"toggleOnOff()\">OFF</button>
        <button class=\"button\" onclick=\"sendOffset(200)\">+200 mV</button>
    </div>
    <script>
        function sendVoltage(voltage) {
            fetch(`/voltage?value=${voltage}`);
        }

        function sendOffset(offset) {
            fetch(`/offset?value=${offset}`);
        }

        // ON/OFFを切り替える関数
        function toggleOnOff() {
            const toggleBtn = document.getElementById(\"toggle-btn\");
            const isCurrentlyOn = toggleBtn.innerText === \"ON\"; // 現在の状態を確認

            // 状態を切り替え
            fetch(\"/toggle?state=\" + (isCurrentlyOn ? \"off\" : \"on\"))
                .then(() => {
                    if (isCurrentlyOn) {
                        toggleBtn.innerText = \"OFF\";
                        toggleBtn.style.backgroundColor = \"red\";
                    } else {
                        toggleBtn.innerText = \"ON\";
                        toggleBtn.style.backgroundColor = \"green\";
                    }
                });
        }

        // 500ms毎に現在の値を更新
        setInterval(() => {
            fetch(\"/current\")
                .then(response => response.text())
                .then(data => {
                    document.getElementById(\"current\").innerText = data;
                });
            fetch(\"/use_class_b\")
                .then(response => response.text())
                .then(data => {
                    const button20V = document.getElementById(\"button20V\");
                    if (data === \"false\") {
                        button20V.disabled = true;
                        button20V.style.backgroundColor = \"#cccccc\";
                    } else {
                        button20V.disabled = false;
                        button20V.style.backgroundColor = \"#fffdd0\";
                    }
                });
        }, 500);
    </script>
</body>
</html>
)rawliteral";

void setupWebUI();

#endif
