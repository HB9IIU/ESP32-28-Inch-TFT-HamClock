
# 🌤️ ESP32 Weather & Time Display

**Author:** HB9IIU  
**For:** Marco T77PM  
**Platform:** ESP32 CYD (Cheap Yellow Display)

> A full-featured weather and dual clock display for ham shacks — local time, UTC time, and real-time weather info, beautifully shown on a 320x240 TFT screen.

---

## 📦 Features

| Feature                    | Description                                                                 |
|----------------------------|-----------------------------------------------------------------------------|
| 🕒 Dual Clocks              | Local time (QTH) and UTC displayed in large, custom 7-segment fonts         |
| 🌐 Accurate Time Sync       | Uses NTP via `pool.ntp.org` for precise timekeeping                         |
| ☀️ Real-Time Weather        | Pulls weather data from OpenWeather API every 5 minutes                     |
| 🖼️ Custom Splash Logo       | Load PNG logos (logo1.png, logo2.png, logo3.png, logo4.png) from SPIFFS     |
| 🧠 Dynamic Config Storage    | Settings are saved in `/settings.json` on SPIFFS and survive reboot         |
| 🖥️ Web Configuration        | Change colors, labels, scroll speed, font styles, boot logo, and more       |
| 🔧 OTA Updates              | Update firmware and filesystem wirelessly                                   |
| 💡 Screensaver Mode         | Activates after 60 minutes of inactivity with a colorful pixel effect       |

---

## 📍 Requirements

| Component            | Description                                     |
|---------------------|-------------------------------------------------|
| ESP32 CYD Board     | Cheap Yellow Display (ILI9341, 320x240 TFT)     |
| Wi-Fi Connection    | Needed for NTP sync and weather API calls       |
| OpenWeather API Key | Get a free key from https://openweathermap.org/ |
| PlatformIO          | Recommended for building and uploading          |

---

## ⚙️ Configuration

Before first upload, edit `config.h` to include your Wi-Fi and weather API key:

```cpp
#define WIFI_SSID      "YourWiFi"
#define WIFI_PASSWORD  "YourPassword"
#define WEATHER_API_KEY "your_api_key_here"
```

> These are fallback values. The system saves user changes to SPIFFS as `/settings.json`.

---

## 🚀 Flash & Upload

```bash
# Build and upload firmware
pio run --target upload

# Upload SPIFFS filesystem (includes PNGs, HTML)
pio run --target uploadfs
```

---

## 🌐 Web Interface

After boot and Wi-Fi connection:

- Access the device via `http://hamclock.local` (if your OS supports mDNS)
- Or check the serial monitor for the assigned IP address

### From the browser, you can:

- ✅ Change time labels (e.g., “Local Time”)
- 🎨 Adjust digit and frame colors
- 🏃 Set banner scroll speed
- ✏️ Toggle italic font style
- 🌍 Update coordinates
- 🖼️ Choose or upload a new splash screen
- 💾 Save all settings to SPIFFS

---

## 🔁 OTA Updates

Update firmware or SPIFFS over the air:

### 1. Edit `platformio.ini`

```ini
upload_protocol = espota
upload_port = hamclock.local  # or use the IP address
```

### 2. Upload firmware
```bash
pio run --target upload
```

### 3. Upload SPIFFS
```bash
pio run --target uploadfs
```

> ⚠️ Don’t keep Serial Monitor open during OTA.

---

## 🖼️ PNG Logo Upload

You can upload your own 320x240 PNG file as the boot splash screen:

- File will be saved as `/logo4.png`
- It will be shown on next boot

---

## 🙏 Credits

Thanks to Marco T77PM for the testing and inspiration.  
Developed with ❤️ by HB9IIU from Switzerland 🇨🇭

---

## 📬 Feedback

Open an issue on GitHub or reach out via ham radio.

**73 de HB9IIU**
