#include "index_html.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <WebServer.h>

// ── CONFIG ───────────────────────────────────────────────
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";

const bool  USE_AP        = false;
const char* AP_SSID       = "KeyPad-ESP32";
const char* AP_PASSWORD   = "keypad123";
// ─────────────────────────────────────────────────────────

USBHIDKeyboard Keyboard;
WebServer      server(80);

void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

String extractJsonString(const String& body, const String& key) {
  String searchKey = "\"" + key + "\"";
  int keyIdx = body.indexOf(searchKey);
  if (keyIdx < 0) return "";
  int colon = body.indexOf(':', keyIdx + searchKey.length());
  if (colon < 0) return "";
  int start = body.indexOf('"', colon + 1);
  if (start < 0) return "";
  int end = start + 1;
  while (end < (int)body.length()) {
    if (body[end] == '\\') { end += 2; continue; }
    if (body[end] == '"')  break;
    end++;
  }
  String result = body.substring(start + 1, end);
  result.replace("\\n", "\n");
  result.replace("\\t", "\t");
  result.replace("\\\"", "\"");
  result.replace("\\\\", "\\");
  return result;
}

int extractJsonInt(const String& body, const String& key, int defaultVal = 30) {
  String searchKey = "\"" + key + "\"";
  int keyIdx = body.indexOf(searchKey);
  if (keyIdx < 0) return defaultVal;
  int colon = body.indexOf(':', keyIdx + searchKey.length());
  if (colon < 0) return defaultVal;
  int start = colon + 1;
  while (start < (int)body.length() && (body[start] == ' ' || body[start] == '\t')) start++;
  String numStr = "";
  for (int i = start; i < (int)body.length(); i++) {
    char c = body[i];
    if (c >= '0' && c <= '9') numStr += c;
    else break;
  }
  return numStr.length() > 0 ? numStr.toInt() : defaultVal;
}

void typeChar(char c) {
  if (c == '\n') {
    Keyboard.press(KEY_RETURN);
    Keyboard.release(KEY_RETURN);
  } else if (c == '\t') {
    Keyboard.press(KEY_TAB);
    Keyboard.release(KEY_TAB);
  } else {
    Keyboard.print(c);
  }
}

void handleRoot() {
  setCORSHeaders();
  server.send_P(200, "text/html", INDEX_HTML);
}

void handlePing() {
  setCORSHeaders();
  server.send(200, "application/json", "{\"ok\":true,\"device\":\"ESP32-S3 KeyPad\"}");
}

void handleType() {
  setCORSHeaders();
  if (server.method() == HTTP_OPTIONS) {
    server.send(204);
    return;
  }
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"No body\"}");
    return;
  }
  String body     = server.arg("plain");
  String text     = extractJsonString(body, "text");
  int    delay_ms = extractJsonInt(body, "delay", 30);

  if (text.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Empty text\"}");
    return;
  }

  delay_ms = constrain(delay_ms, 0, 200);
  Serial.printf("[TYPE] %d chars @ %d ms delay\n", text.length(), delay_ms);
  server.send(200, "application/json", "{\"ok\":true}");

  for (int i = 0; i < (int)text.length(); i++) {
    typeChar(text[i]);
    if (delay_ms > 0) delay(delay_ms);
  }
  Serial.println("[TYPE] Done.");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n== ESP32-S3 KeyPad ==");

  USB.begin();
  Keyboard.begin();
  delay(1000);
  Serial.println("[USB] HID Keyboard ready");

  if (USE_AP) {
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("[WiFi] AP mode  →  http://%s\n", WiFi.softAPIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] Connecting");
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
      delay(500); Serial.print("."); tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\n[WiFi] Connected  →  http://%s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("\n[WiFi] Failed — falling back to AP mode");
      WiFi.softAP(AP_SSID, AP_PASSWORD);
      Serial.printf("[WiFi] AP  →  http://%s\n", WiFi.softAPIP().toString().c_str());
    }
  }

  server.on("/",      HTTP_GET,  handleRoot);
  server.on("/ping",  HTTP_GET,  handlePing);
  server.on("/type",  HTTP_POST, handleType);
  server.on("/type",  HTTP_OPTIONS, [](){
    setCORSHeaders();
    server.send(204);
  });
  server.begin();
  Serial.println("[HTTP] Server started on port 80");
}

void loop() {
  server.handleClient();
}
