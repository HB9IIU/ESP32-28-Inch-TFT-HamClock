# ESP32 HamClock

A **TFT-based ham radio clock & propagation dashboard** for the ESP32.  
It shows **local and UTC time**, **HF/VHF band conditions**, **solar/geomagnetic data**, and **weather info** on a touch-enabled screen.

---

## ✨ Features
- 📺 **TFT Display** — ILI9341 / ILI9488 via **TFT_eSPI** (fast sprites, broad controller support).  
- ⏰ **Dual clocks** — Local time + UTC.  
- 🌤️ **Weather** — OpenWeather API (enter key in web UI).  
- ☀️ **Solar & HF propagation** — Data from [hamqsl.com](https://www.hamqsl.com/).  
- 📡 **Band condition indicators** — Good / Fair / Poor at a glance.  
- 📶 **Easy Wi-Fi setup** — Captive portal AP **`HB9IIUSetup`** on first boot.  
- 🌐 **Web interface** — Configure time labels, colors, banner speed, boot logo; upload custom splash.  
- 💤 **Screensaver mode** — Random pixel animation.  
- 💾 **Persistent settings** — Stored as JSON in SPIFFS.  
- 🔗 **OTA + mDNS** — Update over the air; reach it at **`http://hamclock.local`**.  

---

## 📦 Requirements
- ESP32 (ESP32-S3 recommended for TFT touch projects)  
- TFT compatible with **TFT_eSPI** (ILI9341/ILI9488)  
- OpenWeather API key (optional but recommended)  
- PlatformIO or Arduino IDE  

---

## 🚀 Quick Start

### Option A — Web Installer (no compile)
Use your browser to flash the latest build:  
👉 **ESP32 HamClock Web Installer** — https://esp32.hb9iiu.com/  
(Works with Chrome/Edge and any browser supporting WebSerial.)

### Option B — Manual build
1. Clone the repo and open with **PlatformIO** (or Arduino IDE).  
2. Configure **TFT_eSPI** for your panel (ILI9341/ILI9488).  
3. Build & upload firmware to the ESP32.  

---

## ⚙️ First-time Setup
1. **Power on** the device. It starts an AP called **`HB9IIUSetup`**.  
2. **Connect** to the AP and open `192.168.4.1` to enter your Wi-Fi credentials.  
3. After joining your network, access **`http://hamclock.local`**.  
4. In the web UI, **add your OpenWeather API key**, choose colors, labels, banner speed, and (optionally) **upload a custom splash** image.  
5. Done — the clock/propagation panels will update automatically.  

---

## 🖼️ Screenshots
<p align="center">
  <img src="https://github.com/HB9IIU/ESP32-28-Inch-TFT-HamClock/blob/main/doc/Photos/IMG_8505.png?raw=true" width="600" alt="HamClock Screenshot">
</p>

---

## 🔧 Configuration Tips
- **Display type**: Ensure your **TFT_eSPI** `User_Setup` matches your panel (ILI9341/ILI9488).  
- **OpenWeather**: Create a free API key, paste it in the web UI.  
- **Band indicators**: Values are derived from hamqsl.com solar/geomagnetic data fetched by the device.  
- **Splash screen**: Upload a PNG via the web UI; it’s stored in SPIFFS along with your settings.  

---

## 🙌 Credits / Inspiration
This project was inspired by the excellent work of **SQ9ZAQ**:  
- **HamQSL XML Parser** — https://github.com/canislupus11/HamQSL-XML-Parser  

Thanks for the idea and the initial approach to parsing and displaying **hamqsl.com** propagation data on small TFTs.  
This project is a ground-up implementation for ESP32 with **TFT_eSPI**, a web-configurable UI, captive portal onboarding, and OTA updates.  
No source code from the above project is copied into this repository. (Attribution provided as inspiration.)  

---

## 📝 To-Do
- 📱 Improve web UI layout for **mobile** screens.  

---

## 🧩 Troubleshooting
- **I don’t see the portal `HB9IIUSetup`** — Power cycle; ensure the board isn’t already configured to join your Wi-Fi.  
- **`hamclock.local` doesn’t resolve** — Try the device’s IP from your router; ensure mDNS is supported on your OS/network.  
- **Blank/garbled display** — Reconfirm your **TFT_eSPI** configuration (pins, controller type).  
- **Weather not showing** — Check your OpenWeather API key and network connectivity.  

---

## 📜 License

This project is licensed under the  
**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** license.  

- ✅ You are free to use, modify, and share this work.  
- ✅ You must give **appropriate credit** (attribution).  
- ✅ You must share any derivative works under the **same license**.  
- ❌ You may **not use this work for commercial purposes** (e.g., selling preloaded hardware, reselling code, or monetizing it in any way).  

Full license text: [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)  

---

## 🌐 Web Installer (direct link)
👉 **https://esp32.hb9iiu.com/** — flash from your browser with WebSerial.  
