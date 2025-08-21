# ESP32 HamClock

This project is a **TFT-based ham radio clock and propagation display** for the ESP32.  
It shows **local and UTC time**, **HF/VHF band conditions**, **solar/geomagnetic data**, and **weather info** on a touch-enabled screen.

---

## ✨ Features
- 📺 **TFT Display (ILI9341/ILI9488 via TFT_eSPI)**
- ⏰ **Dual clocks**: Local time and UTC time
- 🌤️ **Weather data** from OpenWeather API (requires API key)
- ☀️ **Solar & HF propagation data** from [hamqsl.com](https://www.hamqsl.com/)
- 📡 **Band condition indicators** (Good / Fair / Poor)
- 📶 **Wi-Fi setup via captive portal** (AP mode `HB9IIUSetup`)
- 🌐 **Web interface** for configuration:
  - Time labels
  - Colors
  - Banner speed
  - Boot logo selection
- 🖼️ **Custom splash screen upload** via web
- 💤 **Screensaver mode** with random pixel animation
- 🔧 **Settings saved in SPIFFS** (JSON file)
- 🔗 **OTA updates + mDNS (`http://hamclock.local`)**

---

## 📦 Requirements
- ESP32 (ESP32-S3 recommended with TFT touch support)
- TFT display compatible with [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- OpenWeather API key (for weather info)
- PlatformIO or Arduino IDE

---

## 🚀 Getting Started
1. Flash the firmware to your ESP32.
2. On first boot, connect to the Wi-Fi AP **`HB9IIUSetup`** and open `192.168.4.1` to enter your Wi-Fi credentials.
3. Access the device at [http://hamclock.local](http://hamclock.local) once connected.
4. Enter your OpenWeather API key via the web UI.
5. Enjoy real-time ham radio propagation and clock information!

---

## 🖼️ Screenshots / Demo
<p align="center">
  <img src="https://github.com/HB9IIU/ESP32-28-Inch-TFT-HamClock/blob/main/doc/Photos/IMG_8505.png?raw=true" width="600" alt="HamClock Screenshot">
</p>

---

## 🌐 Web Installer
No need to compile anything! You can flash the latest version of ESP32 HamClock directly from your browser using the  
👉 [**ESP32 HamClock Web Installer**](https://esp32.hb9iiu.com/)  

Works with Chrome/Edge and any browser that supports WebSerial.

---

## 📝 To-Do
- 📱 Adapt Web UI for better viewing on **mobile phones**

---

## 📜 License
This project is provided under the MIT License. See [LICENSE](LICENSE) for details.
