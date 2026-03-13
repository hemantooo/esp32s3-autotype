# ⌨️ ESP32-S3 AutoType

> Type anything on your PC using your phone — no software needed on the computer.

ESP32-S3 acts as a **USB HID keyboard** plugged into your laptop. Connect your phone to the same WiFi, open a browser, paste your text, and hit send. The ESP32 types it out automatically.

---

## 📱 How It Works

```
Your Phone (Browser UI)
        │
        │  WiFi / HTTP POST
        ▼
   ESP32-S3 Board
        │
        │  USB HID Keyboard
        ▼
 Ubuntu / Any PC
```

1. ESP32-S3 connects to your WiFi and starts a tiny web server
2. Open `http://<esp32-ip>` on your phone
3. Type or paste text → tap **"Type it"**
4. ESP32 types it on your PC instantly

---

## 🛒 What You Need

| Item | Notes |
|------|-------|
| ESP32-S3 Dev Board | Any variant with native USB port |
| USB-C Cable | Must be a **data cable**, not charge-only |
| Ubuntu / Any PC | Receives keystrokes via USB |
| Smartphone | Any browser — Chrome, Firefox, Safari |
| Arduino IDE 2.x | For flashing the firmware |

---

## 🚀 Installation & Setup

### Step 1 — Install Arduino IDE

Download Arduino IDE 2.x from [arduino.cc/en/software](https://www.arduino.cc/en/software)

```bash
chmod +x arduino-ide_*.AppImage
./arduino-ide_*.AppImage
```

Add your user to the serial port group (Linux only):
```bash
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```
**Log out and log back in** for this to take effect.

---

### Step 2 — Install ESP32-S3 Board Support

1. Open Arduino IDE → **File → Preferences**
2. Paste this URL in *Additional boards manager URLs*:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
3. Go to **Tools → Board → Boards Manager**
4. Search `esp32` → install **esp32 by Espressif Systems** (version 2.0.5 or newer)

---

### Step 3 — Configure Board Settings

Go to **Tools** menu and set each option exactly:

| Setting | Value |
|---------|-------|
| Board | `ESP32S3 Dev Module` |
| USB Mode | `USB OTG` ⚠️ Critical |
| USB CDC On Boot | `Enabled` |
| USB Firmware MSC On Boot | `Disabled` |
| USB DFU On Boot | `Disabled` |
| Upload Mode | `UART0 / Hardware CDC` |
| CPU Frequency | `240MHz (WiFi)` |
| Flash Size | `4MB (32Mb)` |
| Partition Scheme | `Default 4MB with spiffs` |

> ⚠️ **USB Mode must be set to `USB OTG`** — this is what enables HID keyboard functionality. Any other setting will not work.

---

### Step 4 — Clone & Configure

```bash
git clone https://github.com/hemantooo/esp32s3-autotype.git
cd esp32s3-autotype
```

Open `esp32s3-autotype.ino` in Arduino IDE and edit your WiFi credentials at the top:

```cpp
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
```

**Or use hotspot mode** (no router needed) — set:
```cpp
const bool USE_AP = true;
```
Then connect your phone directly to the `KeyPad-ESP32` hotspot.

---

### Step 5 — Flash the Firmware

Your ESP32-S3 has two USB ports:

| Port | Use |
|------|-----|
| `COM` | Uploading / flashing code |
| `USB` | Running as HID keyboard |

1. Plug into the **COM port**
2. In Arduino IDE → **Tools → Port** → select `/dev/ttyACM0`
3. Click **Upload** (Ctrl+U)

> If upload fails: hold **BOOT** button → press **RESET** → release **BOOT** → try upload again

---

### Step 6 — Run It

1. Unplug from COM port
2. Plug into **USB port**
3. Open **Serial Monitor** (baud: 115200) → press RESET
4. You'll see:
```
[USB] HID Keyboard ready
[WiFi] Connected  →  http://192.168.x.x
[HTTP] Server started on port 80
```
5. Note the IP address

---

### Step 7 — Use from Your Phone

1. Connect phone to same WiFi as ESP32
2. Open browser → go to `http://192.168.x.x`
3. Green dot = **ready** ✅
4. Click inside any app on your PC → type text on phone → tap **"Type it"**

---

## 📲 Web UI Features

| Feature | Description |
|---------|-------------|
| **Text input** | Type or paste any text |
| **Typing speed** | Slider from 0ms (fast) to 100ms (slow) |
| **Saved snippets** | Save frequent phrases for one-tap access |
| **Status indicator** | Green = ready, Orange = typing |

---

## 🗂️ Project Structure

```
esp32s3-autotype/
├── esp32s3-autotype.ino   # Main firmware
├── index_html.h           # Web UI served by ESP32
└── README.md
```

---

## 🔧 Troubleshooting

| Problem | Fix |
|---------|-----|
| Port greyed out in Arduino IDE | Plug into COM port, not USB port |
| Upload fails | Hold BOOT + press RESET to enter download mode |
| No HID keyboard detected | Check USB Mode = `USB OTG` in Tools menu |
| Wrong characters typed | Set keyboard layout to US in system settings |
| Phone shows "checking…" | Make sure phone & ESP32 are on same WiFi |
| Characters get skipped | Increase typing delay slider to 40–60ms |

---

## 📄 License

MIT License — free to use, modify, and share.

---

<p align="center">Built with ❤️ using ESP32-S3 + Arduino</p>
