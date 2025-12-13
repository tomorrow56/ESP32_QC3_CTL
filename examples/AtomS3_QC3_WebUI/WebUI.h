#ifndef WEBUI_H
#define WEBUI_H

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32_QC3_CTL.h>
#include "PinDefinitions.h"

// グローバル変数の宣言
extern WebServer server;
extern ESP32_QC3_CTL qc3;
extern bool isOn;      // ON/OFF状態のフラグ
extern bool isQcVal;   // 連続モードフラグ

// HTMLページの定義
const char* const htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>QuickCharge 3.0 Control</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 12px; font-size: 18px; }
        h1 { margin: 12px 0; font-size: 34px; line-height: 1.15; word-break: break-word; }
        .button-grid { display: flex; flex-wrap: wrap; justify-content: center; gap: 12px; margin: 10px 0; }
        .button { padding: 16px 0; font-size: 32px; border: 1px solid #ccc; border-radius: 5px; cursor: pointer; background-color: #fffdd0; box-sizing: border-box; }
        a.button { display: inline-block; text-decoration: none; color: inherit; }
        .button { width: 42vw; max-width: 200px; min-width: 140px; }
        .button.wide { width: 88vw; max-width: 420px; }
        .button.toggle { width: 60vw; max-width: 260px; }
        .button:hover { background-color: #fffdd0; }
        .on-off { background-color: red; color: white; } /* 初期状態は緑 */
        .voltage-label { font-size: 28px; }
        .voltage-value { font-size: 34px; color: blue; font-weight: bold; }
        #status { font-size: 16px; color: #333; margin-top: 8px; }

        @media (min-width: 600px) {
            body { padding: 0; font-size: 24px; }
            h1 { font-size: 48px; margin: 16px 0; }
            .button { font-size: 48px; padding: 20px 0; width: 220px; max-width: 220px; }
            .button.wide { width: 420px; max-width: 420px; }
            .button.toggle { width: 260px; max-width: 260px; }
            .voltage-label { font-size: 48px; }
            .voltage-value { font-size: 48px; }
            #status { font-size: 24px; margin-top: 10px; }
        }
    </style>
</head>
<body>
    <h1>QuickCharge 3.0 Control</h1>
    <div>
        <label class="voltage-label">Output: </label>
        <span id="current" class="voltage-value">0</span>
        <label class="voltage-label"> mV</label>
    </div>
    <div id="status"></div>
    <br>
    <div class="button-grid">
        <a class="button" href="/voltage?value=5" onclick="if (typeof sendVoltage === 'function') { sendVoltage(5); return false; }">5 V</a>
        <a class="button" href="/voltage?value=9" onclick="if (typeof sendVoltage === 'function') { sendVoltage(9); return false; }">9 V</a>
        <a class="button" href="/voltage?value=12" onclick="if (typeof sendVoltage === 'function') { sendVoltage(12); return false; }">12 V</a>
        <a class="button" href="/voltage?value=20" onclick="if (typeof sendVoltage === 'function') { sendVoltage(20); return false; }" id="button20V">20 V</a>
    </div>
    <div class="button-grid">
        <a class="button wide" href="/offset?value=-200" onclick="if (typeof sendOffset === 'function') { sendOffset(-200); return false; }">-200 mV</a>
        <a id="toggle-btn" class="button toggle on-off" href="/toggle?state=on" onclick="toggleOnOff(); return false;">OFF</a>
        <a class="button wide" href="/offset?value=200" onclick="if (typeof sendOffset === 'function') { sendOffset(200); return false; }">+200 mV</a>
    </div>
    <script>
        function setStatus(text) {
            var statusEl = document.getElementById('status');
            if (statusEl) {
                statusEl.innerText = text;
            }
        }

        setStatus('ready');

        var uiIsOn = false;

        function applyOnOffState(isOn) {
            uiIsOn = isOn;
            var toggleBtn = document.getElementById("toggle-btn");
            if (!toggleBtn) {
                return;
            }
            if (uiIsOn) {
                toggleBtn.innerText = "ON";
                toggleBtn.style.backgroundColor = "green";
            } else {
                toggleBtn.innerText = "OFF";
                toggleBtn.style.backgroundColor = "red";
            }
        }

        function refreshOnOff() {
            xhrGet('/state', function (status, data) {
                if (status === 200) {
                    applyOnOffState(String(data).trim() === 'on');
                }
            });
        }

        function refreshCurrent() {
            xhrGet('/current', function (status, data) {
                if (status === 200) {
                    var currentEl = document.getElementById("current");
                    if (currentEl) {
                        currentEl.innerText = data;
                    }
                }
            });
        }

        function withTs(url) {
            var ts = Date.now();
            return url + (url.indexOf('?') >= 0 ? '&' : '?') + 'ts=' + ts;
        }

        function xhrGet(url, cb) {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function () {
                if (xhr.readyState === 4) {
                    cb(xhr.status, xhr.responseText);
                }
            };
            xhr.open('GET', withTs(url), true);
            xhr.send(null);
        }

        function sendVoltage(voltage) {
            setStatus('send voltage: ' + voltage);
            xhrGet('/voltage?value=' + voltage, function (status) {
                if (status === 200) {
                    setStatus('OK: voltage ' + voltage);
                    refreshCurrent();
                } else {
                    setStatus('ERR: voltage ' + voltage + ' / HTTP ' + status);
                }
            });
        }

        function sendOffset(offset) {
            setStatus('send offset: ' + offset);
            xhrGet('/offset?value=' + offset, function (status) {
                if (status === 200) {
                    setStatus('OK: offset ' + offset);
                    refreshCurrent();
                } else {
                    setStatus('ERR: offset ' + offset + ' / HTTP ' + status);
                }
            });
        }

        // ON/OFFを切り替える関数
        function toggleOnOff() {
            var nextState = uiIsOn ? 'off' : 'on';

            setStatus('toggle: ' + nextState);
            xhrGet('/toggle?state=' + nextState, function (status) {
                if (status === 200) {
                    setStatus('OK: toggle ' + nextState);
                    refreshOnOff();
                    refreshCurrent();
                } else {
                    setStatus('ERR: toggle ' + nextState + ' / HTTP ' + status);
                }
            });
        }

        // 500ms毎に現在の値を更新
        setInterval(function () {
            xhrGet('/current', function (status, data) {
                if (status === 200) {
                    document.getElementById("current").innerText = data;
                }
            });
            refreshOnOff();
            xhrGet('/use_class_b', function (status, data) {
                if (status === 200) {
                    var button20V = document.getElementById("button20V");
                    if (data === "false") {
                        button20V.disabled = true;
                        button20V.style.backgroundColor = "#cccccc";
                    } else {
                        button20V.disabled = false;
                        button20V.style.backgroundColor = "#fffdd0";
                    }
                }
            });
        }, 2000);

        refreshOnOff();
        refreshCurrent();
    </script>
</body>
</html>
)rawliteral";

void setupWebUI();

#endif
