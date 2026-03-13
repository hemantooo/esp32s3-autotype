#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <WebServer.h>

// ── CONFIG ───────────────────────────────────────────────
// Option A: Join existing WiFi (phone + ESP32 on same network)
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Option B: ESP32 creates its own hotspot — set USE_AP = true
//           Then connect your phone to "KeyPad-ESP32" / password below
const bool  USE_AP        = false;
const char* AP_SSID       = "KeyPad-ESP32";
const char* AP_PASSWORD   = "keypad123";
// ─────────────────────────────────────────────────────────

USBHIDKeyboard Keyboard;
WebServer      server(80);

// ── HTML served to the phone ──────────────────────────────
// (minified at build time; full source in web/index.html)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
    <title>KeyPad</title>
    <link
        href="https://fonts.googleapis.com/css2?family=DM+Mono:wght@400;500&family=DM+Sans:wght@300;400;500&display=swap"
        rel="stylesheet">
    <style>
        :root {
            --bg: #f5f4f0;
            --surface: #ffffff;
            --ink: #1a1a18;
            --muted: #8a8a82;
            --accent: #2d5a27;
            --accent-light: #e8f0e6;
            --border: #ddddd5;
            --radius: 12px
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            -webkit-tap-highlight-color: transparent
        }

        body {
            background: var(--bg);
            color: var(--ink);
            font-family: 'DM Sans', sans-serif;
            min-height: 100dvh;
            padding: 20px 16px 32px;
            max-width: 480px;
            margin: 0 auto
        }

        header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 28px;
            padding-top: 8px
        }

        .logo {
            font-family: 'DM Mono', monospace;
            font-size: 15px;
            font-weight: 500;
            letter-spacing: -.3px
        }

        .logo span {
            color: var(--accent)
        }

        .status {
            display: flex;
            align-items: center;
            gap: 6px;
            font-size: 12px;
            color: var(--muted);
            font-family: 'DM Mono', monospace
        }

        .dot {
            width: 7px;
            height: 7px;
            border-radius: 50%;
            background: #ccc;
            transition: background .4s
        }

        .dot.ok {
            background: #4caf50
        }

        .dot.busy {
            background: #ff9800;
            animation: pulse 1s infinite
        }

        @keyframes pulse {

            0%,
            100% {
                opacity: 1
            }

            50% {
                opacity: .4
            }
        }

        .card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            padding: 16px;
            margin-bottom: 14px
        }

        label {
            display: block;
            font-size: 11px;
            font-weight: 500;
            letter-spacing: .6px;
            text-transform: uppercase;
            color: var(--muted);
            margin-bottom: 8px;
            font-family: 'DM Mono', monospace
        }

        textarea {
            width: 100%;
            border: none;
            outline: none;
            resize: none;
            font-family: 'DM Mono', monospace;
            font-size: 14px;
            line-height: 1.65;
            color: var(--ink);
            background: transparent;
            min-height: 140px
        }

        textarea::placeholder {
            color: #c5c5bb
        }

        .row {
            display: flex;
            gap: 10px;
            align-items: center
        }

        .speed-wrap {
            flex: 1
        }

        input[type=range] {
            width: 100%;
            accent-color: var(--accent);
            height: 3px;
            cursor: pointer
        }

        .speed-val {
            font-family: 'DM Mono', monospace;
            font-size: 12px;
            color: var(--muted);
            min-width: 48px;
            text-align: right
        }

        .btn-send {
            width: 100%;
            padding: 15px;
            background: var(--ink);
            color: #fff;
            border: none;
            border-radius: var(--radius);
            font-family: 'DM Sans', sans-serif;
            font-size: 15px;
            font-weight: 500;
            cursor: pointer;
            letter-spacing: -.2px;
            transition: opacity .15s, transform .1s;
            margin-bottom: 14px
        }

        .btn-send:active {
            opacity: .8;
            transform: scale(.985)
        }

        .btn-send.busy {
            background: var(--muted)
        }

        .snippets-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px
        }

        .snip {
            background: var(--accent-light);
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 10px 12px;
            font-size: 12px;
            font-family: 'DM Mono', monospace;
            cursor: pointer;
            color: var(--accent);
            transition: border-color .15s;
            text-align: left;
            word-break: break-word;
            line-height: 1.4
        }

        .snip:active {
            border-color: var(--accent)
        }

        .add-snip {
            background: transparent;
            border: 1px dashed var(--border);
            border-radius: 8px;
            padding: 10px 12px;
            font-size: 12px;
            font-family: 'DM Mono', monospace;
            cursor: pointer;
            color: var(--muted);
            text-align: center
        }

        .toast {
            position: fixed;
            bottom: 28px;
            left: 50%;
            transform: translateX(-50%) translateY(20px);
            background: var(--ink);
            color: #fff;
            font-family: 'DM Mono', monospace;
            font-size: 12px;
            padding: 10px 18px;
            border-radius: 99px;
            opacity: 0;
            transition: opacity .25s, transform .25s;
            pointer-events: none;
            white-space: nowrap
        }

        .toast.show {
            opacity: 1;
            transform: translateX(-50%) translateY(0)
        }
    </style>
</head>

<body>
    <header>
        <div class="logo">Key<span>Pad</span></div>
        <div class="status">
            <div class="dot" id="dot"></div><span id="statusTxt">checking…</span>
        </div>
    </header>

    <div class="card">
        <label>Your text</label>
        <textarea id="txt" placeholder="Paste or type anything here…" rows="6"></textarea>
    </div>

    <div class="card">
        <label>Typing speed</label>
        <div class="row">
            <div class="speed-wrap">
                <input type="range" id="speed" min="0" max="100" value="50">
            </div>
            <div class="speed-val" id="speedLabel">50 ms</div>
        </div>
    </div>

    <button class="btn-send" id="sendBtn" onclick="sendText()">⌨ Type it</button>

    <div class="card">
        <label>Saved snippets</label>
        <div class="snippets-grid" id="snippetGrid"></div>
    </div>

    <div class="toast" id="toast"></div>

    <script>
        const snippets = JSON.parse(localStorage.getItem('snips') || '["Hello, World!","sudo apt update","git status","ls -la"]');
        let busy = false;

        function renderSnippets() {
            const g = document.getElementById('snippetGrid');
            g.innerHTML = snippets.map((s, i) =>
                `<button class="snip" onclick="useSnip(${i})">${s}</button>`
            ).join('') +
                `<button class="add-snip" onclick="addSnip()">+ add</button>`;
        }

        function useSnip(i) {
            document.getElementById('txt').value = snippets[i];
            showToast('Snippet loaded');
        }

        function addSnip() {
            const t = document.getElementById('txt').value.trim();
            if (!t) { showToast('Type something first'); return; }
            const label = prompt('Save snippet as:', t.slice(0, 30));
            if (label === null) return;
            snippets.push(label || t.slice(0, 30));
            localStorage.setItem('snips', JSON.stringify(snippets));
            renderSnippets();
            showToast('Snippet saved!');
        }

        document.getElementById('speed').addEventListener('input', function () {
            document.getElementById('speedLabel').textContent = this.value + ' ms';
        });

        async function sendText() {
            if (busy) return;
            const text = document.getElementById('txt').value;
            if (!text.trim()) { showToast('Nothing to type!'); return; }
            const speed = document.getElementById('speed').value;
            busy = true;
            const btn = document.getElementById('sendBtn');
            btn.textContent = '⏳ Typing…';
            btn.classList.add('busy');
            setStatus('busy', 'typing…');
            try {
                const res = await fetch('/type', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ text, delay: parseInt(speed) })
                });
                const data = await res.json();
                if (data.ok) showToast('✓ Done!');
                else showToast('Error: ' + (data.error || 'unknown'));
            } catch (e) {
                showToast('Connection lost');
            } finally {
                busy = false;
                btn.textContent = '⌨ Type it';
                btn.classList.remove('busy');
                setStatus('ok', 'ready');
            }
        }

        function setStatus(state, txt) {
            document.getElementById('dot').className = 'dot ' + state;
            document.getElementById('statusTxt').textContent = txt;
        }

        function showToast(msg) {
            const t = document.getElementById('toast');
            t.textContent = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 2200);
        }

        async function checkPing() {
            try {
                const r = await fetch('/ping');
                const d = await r.json();
                setStatus('ok', 'ready');
            } catch (e) {
                setStatus('', 'offline');
            }
        }

        renderSnippets();
        checkPing();
    </script>
</body>

</html>
)rawliteral";

// ── Helpers ───────────────────────────────────────────────

void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Simple JSON parser — avoids heavy ArduinoJson dependency
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
  // unescape basic sequences
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

// Type a single character with HID
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

// ── Route handlers ────────────────────────────────────────

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
  String body  = server.arg("plain");
  String text  = extractJsonString(body, "text");
  int    delay_ms = extractJsonInt(body, "delay", 30);

  if (text.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Empty text\"}");
    return;
  }

  // Clamp delay: 0 ms = fast, 100 ms = slow
  delay_ms = constrain(delay_ms, 0, 200);

  Serial.printf("[TYPE] %d chars @ %d ms delay\n", text.length(), delay_ms);

  // Respond immediately so phone doesn't time out on long texts
  server.send(200, "application/json", "{\"ok\":true}");

  // Now type — after HTTP response is sent
  for (int i = 0; i < (int)text.length(); i++) {
    typeChar(text[i]);
    if (delay_ms > 0) delay(delay_ms);
  }
  Serial.println("[TYPE] Done.");
}

// ── Setup & Loop ──────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n== ESP32-S3 KeyPad ==");

  // Start USB HID keyboard
  USB.begin();
  Keyboard.begin();
  delay(1000); // let USB enumerate on host
  Serial.println("[USB] HID Keyboard ready");

  // WiFi
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

  // HTTP routes
  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/ping",   HTTP_GET,  handlePing);
  server.on("/type",   HTTP_POST, handleType);
  server.on("/type",   HTTP_OPTIONS, [](){
    setCORSHeaders();
    server.send(204);
  });
  server.begin();
  Serial.println("[HTTP] Server started on port 80");
}

void loop() {
  server.handleClient();
}
