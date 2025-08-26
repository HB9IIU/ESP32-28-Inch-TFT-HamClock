
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <HB9IIU7seg42ptItalic.h> // https://rop.nl/truetype2gfx/ https://fontforge.org/en-US/
#include <HB9IIUOrbitronMed8pt.h>
#include <HB9IIOrbitronMed10pt.h>
#include <HB9IIU7seg42ptNormal.h>
#include <HB97DIGITS12pt7b.h>
#include "qrcode.h"
#include <UbuntuMono_Regular8pt7b.h>
#include <JetBrainsMono_Bold15pt7b.h>
#include <JetBrainsMono_Light7pt7b.h>
#include <JetBrainsMono_Bold11pt7b.h>
#include <tinyxml2.h>
#include <PNGdec.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Preferences.h>
#include "html_page.h"
#include "html_success.h"
#include "digits60pt7b.h"

String apiKey;
const byte DNS_PORT = 53;
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);

static unsigned long lastDotUpdate = 0;                 // for screen saver
static unsigned long nextDotDelay = random(1000, 2001); // for screen saver
unsigned long currentMillis = millis();
unsigned long lastActivity = 0;                    // Last time user interacted (for screensaver)
unsigned long screenSaverTimeout = 1000 * 60 * 60; // 60 minute
bool useScreenSaver = false;
bool successFullTimeUpdate = false;
int tOffset = 0; // will be updated via configuration device time (Iphone) and later via API call that contains offset according to lat & lon
Preferences prefs;
// --- globals ---
uint8_t activePage = 1;
const uint8_t MAX_PAGES = 7;
unsigned long lastTouchMs = 0;
bool wasTouching = false;
int scanCount = 0;
bool inAPmode = false;
bool autoPageChange = false;
char bigClockLastDigit[4] = {' ', ' ', ' ', ' '};
// Struct to store all parsed solar data
struct SolarData
{
    String source;
    String updated;
    int solarFlux;
    int aIndex;
    int kIndex;
    String kIndexNT;
    String xRay;
    int sunspots;
    float heliumLine;
    String protonFlux;
    String electronFlux;
    int aurora;
    float normalization;
    float latDegree;
    float solarWind;
    float magneticField;
    String geomagneticField;
    String signalNoise;
    String fof2;
    String mufFactor;
    String muf;

    struct BandCondition
    {
        String name;
        String time;
        String condition;
    } bandConditions[8];

    struct VHFCondition
    {
        String name;
        String location;
        String condition;
    } vhfConditions[5];
};

SolarData solarData;

// Create web server
WebServer server(80); // HTTP server on port 80

// Configurable Settings (replace all previous #defines)
bool APIkeyIsValid = false;
float latitude = 46.4667118;
float longitude = 6.8590456;
uint16_t localTimeColour = TFT_GREEN;
uint16_t utcTimeColour = TFT_GOLD;
bool doubleFrame = false;
uint16_t localFrameColour = TFT_DARKGREY;
uint16_t utcFrameColour = TFT_DARKGREY;
uint16_t bannerColour = TFT_DARKGREEN;
uint16_t bigClockColour = TFT_GREEN;

int bannerSpeed = 5;
String localTimeLabel = "  QTH Time  ";
String utcTimeLabel = "  UTC Time  ";
String startupLogo = "logo3.png";
bool italicClockFonts = false;

volatile bool refreshDigits = false;
const String weatherAPI = "https://api.openweathermap.org/data/2.5/weather"; // OpenWeather API endpoint

// Global variables for previous time tracking
String previousLocalTime = "";
String previousUTCtime = "";
int refreshDigitsCounter = 0;
volatile bool refreshFrames = false;
int refreshFramesCounter = 0;

// for solar data to be cleaned
const char *solarDataUrl = "https://www.hamqsl.com/solarxml.php";
String formatUpdatedTimestampToUTC(const String &raw);

String LOCALlastTimeStr = "        "; // 8 characters: HH:MM:SS
String UTClastTimeStr = "        ";   // 8 characters: HH:MM:SS
String LASTbigClockTimeStr = "";
uint16_t LOCALdigitColor = TFT_LIGHTGREY;
uint16_t UTCdigitColor = TFT_LIGHTGREY;
bool blinkingDot = false; // colons on Propagation page clocks
bool colonVisible = true; // global var used to show/hide colons on Propagation page clocks
bool redrawMainPropagationPage = true;
bool redrawSolarSummaryPage1 = true;
bool redrawSolarSummaryPage2 = true;
bool redrawSolarSummaryPage3 = true;
bool reDrawWiFiQualityPage = true;

// Relative x-offsets for HB97DIGITS12pt7b font layout
const int xOffsets[8] = {
    0,  // H1
    15, // H2
    30, // :
    36, // M1
    51, // M2
    66, // :
    72, // S1
    87  // S2
};

// TFT Display Setup
TFT_eSPI tft = TFT_eSPI();                     // Create TFT display object
TFT_eSprite scrollingText = TFT_eSprite(&tft); // Sprite object for "Hello World" text

TFT_eSprite labelSprite = TFT_eSprite(&tft); // Global sprite

// Scrolling Text
int scrollingTextXposition;                                                                                                                        // Variable for text position (to start at the rightmost side)
String scrollText = "Sorry, No Weather Info At This Moment!!!    Have you enterred your API key via the Web Interface at http://hamclock.local ?"; // Text to scroll
// Timing variables
unsigned long previousMillisForScroller = 0; // Store last time the action was performed

// NTP Client Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 15000); // UTC offset and update interval

// Function Prototypes
void saveSettings();
void drawQRCode(const char *text, int x, int y, int scale);
void drawQRcodeInstructions();
void startConfigurationPortal();
bool tryToConnectSavedWiFi();
void fetchWeatherData();
String formatLocalTime(long epochTime);
String convertEpochToTimeString(long epochTime);
void displayTime(int x, int y, String time, String &previousTime, int yOffset, uint16_t fontColor);
String convertTimestampToDate(long timestamp);
void loadSettings();
void handleRoot();
void handleApiKeyPage();
void handleSaveApiKey();
void handleGetApiKey();
void retrieveAPIkeyFromPref();
void handleSave();
void drawOrredrawStaticElements();
void mountAndListSPIFFS(uint8_t levels = 255, bool listContent = true);
void handlePNGUpload();
void handleTouchToRotatePage();
void drawMainPropagationPage();
void drawSolarSummaryPage1();
void drawSolarSummaryPage2();
void drawSolarSummaryPage3();
void drawWiFiQualityPage();
void redrawdrawMainPropagationPagePage1();
void fetchSolarData();
void drawLOCALTime(const String &timeStr, int x, int y, uint16_t digitColor, uint16_t backgroundColor, bool blinkColon);
void drawUTCTime(const String &timeStr, int x, int y, uint16_t digitColor, uint16_t backgroundColor, bool blinkColon);
void updateWiFiSignalDisplay();
void drawWiFiSignalMeter(int qualityPercent);
void handleRootCaptivePortal();
void handleScanCaptivePortal();
void handleSaveCaptivePortal();
void startConfigurationPortal();
void checkIfscreenIsTouchedDuringStartUpForFactoryReset();
void tryToRetrieveUTCoffsetFromFirstConfiguration();
// PNG Decoder Setup
PNG png;
fs::File pngFile; // Global File handle (required for PNGdec callbacks)

// Callback functions for PNGdec
void *fileOpen(const char *filename, int32_t *size);
void fileClose(void *handle);
int32_t fileRead(PNGFILE *handle, uint8_t *buffer, int32_t length);
int32_t fileSeek(PNGFILE *handle, int32_t position);
void displayPNGfromSPIFFS(const char *filename, int duration_ms);

void setup()
{

    // Start Serial Monitor
    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Backlight pin setup
    pinMode(TFT_BLP, OUTPUT);
    digitalWrite(TFT_BLP, HIGH); // Turn backlight ON permanently

    //   Initialize TFT display
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    Serial.println("TFT Display initialized!");
    tft.setFreeFont(&digits60pt7b);

    checkIfscreenIsTouchedDuringStartUpForFactoryReset();
    // 🔧 Mount SPIFFS
    mountAndListSPIFFS();
    // Load saved settings first
    loadSettings();
    // Display PNG from SPIFFS
    displayPNGfromSPIFFS(startupLogo.c_str(), 0);
    // BETA release display
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString("Beta Pre-Release", 160, 210, 1);

    retrieveAPIkeyFromPref();
    // apiKey="";

    labelSprite.setColorDepth(8);
    labelSprite.createSprite(120, 30); // Size depends on font & text
    labelSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
    labelSprite.setTextDatum(MC_DATUM);
    labelSprite.setFreeFont(&FreeSansBold12pt7b);

    if (!tryToConnectSavedWiFi())
    {
        inAPmode = true;

        startConfigurationPortal();
    }

    if (inAPmode == false)
    {
        // Start mDNS
        if (!MDNS.begin("hamclock"))
        {
            Serial.println("⚠️ Failed to start mDNS responder!");
        }
        else
        {
            Serial.println("🌍 mDNS started successfully. You can access via http://hamclock.local");
        }

        {
            // Start Web Server
            server.on("/", handleRoot);                     // Serve the HTML page
                                                            // Serve all static files (HTML, PNG, CSS, etc.)
            server.serveStatic("/fonts", SPIFFS, "/fonts"); // optional
            server.serveStatic("/logo1.png", SPIFFS, "/logo1.png");
            server.serveStatic("/logo2.png", SPIFFS, "/logo2.png");
            server.serveStatic("/logo3.png", SPIFFS, "/logo3.png");
            server.serveStatic("/logo4.png", SPIFFS, "/logo4.png");
            server.serveStatic("/logo4.png", SPIFFS, "/logo4.png");
            server.serveStatic("/github.png", SPIFFS, "/github.png");
            server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
            server.on("/config", HTTP_GET, []()
                      {
  JsonDocument doc;

  doc["latitude"] = latitude;
  doc["longitude"] = longitude;
  doc["localTimeColour"] = localTimeColour;
  doc["utcTimeColour"] = utcTimeColour;
  doc["doubleFrame"] = doubleFrame;
  doc["localFrameColour"] = localFrameColour;
  doc["utcFrameColour"] = utcFrameColour;
  doc["bannerColour"] = bannerColour;
  doc["bannerSpeed"] = bannerSpeed;
  doc["localTimeLabel"] = localTimeLabel;
  doc["utcTimeLabel"] = utcTimeLabel;
  doc["startupLogo"] = startupLogo;
  doc["italicClockFonts"] = italicClockFonts;
doc["screenSaverTimeout"] = screenSaverTimeout / 60000;  // convert ms → minutes
doc ["APIkeyIsValid"] =APIkeyIsValid;
doc ["autoPageChange"] =autoPageChange;

 Serial.println(APIkeyIsValid);
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response); });

            server.on("/scrolltext", []()
                      { server.send(200, "text/plain", scrollText); });

            server.on("/setcolor", HTTP_POST, []()
                      {


   JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON parse error");
        return;
    }

    String target = doc["target"];

    // ✅ Handle doubleFrame checkbox
    if (target == "doubleFrame") {
        bool thinBorder = doc["value"];
        doubleFrame = !thinBorder; // Inverse logic
        Serial.printf("🪟 doubleFrame set to: %s (thinBorder: %s)\n", doubleFrame ? "true" : "false", thinBorder ? "true" : "false");
        server.send(200, "text/plain", "OK");
        return;
    }

    // ✅ All other color-based updates
    uint16_t color = doc["color"];

    if (target == "localTimeDigits") {
        localTimeColour = color;
        Serial.printf("🎨 localTimeDigits set to: 0x%04X\n", color);
    } else if (target == "localTimeFrame") {
        localFrameColour = color;
        Serial.printf("🖼️ localTimeFrame set to: 0x%04X\n", color);
    } else if (target == "utcTimeDigits") {
        utcTimeColour = color;
        Serial.printf("🎨 utcTimeDigits set to: 0x%04X\n", color);
    } else if (target == "utcTimeFrame") {
        utcFrameColour = color;
        Serial.printf("🖼️ utcTimeFrame set to: 0x%04X\n", color);
    } else if (target == "weatherBannerText") {
        bannerColour = color;
        Serial.printf("🟩 bannerColour set to: 0x%04X\n", color);
    }      else if (target == "bigClockTime") {
        bigClockColour = color;
        LASTbigClockTimeStr="";
for (int i = 0; i < 4; i++) {
    bigClockLastDigit[i] = ' '; 
}        Serial.printf("🟩 big clock color set to: 0x%04X\n", color); }
    
    else {
        Serial.printf("⚠️ Unknown target: %s\n", target.c_str());
        server.send(400, "text/plain", "Unknown target");
        return;
    }
    saveSettings();
    if (activePage==1){
    drawOrredrawStaticElements();
    }
    refreshDigits = true;
    server.send(200, "text/plain", "OK"); });

            server.on("/setspeed", HTTP_POST, []()
                      {

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON parse error");
        return;
    }

    int speed = doc["speed"];  // This will already be 45 - slider
    bannerSpeed = constrain(speed, 0, 45);
    Serial.printf("🎬 bannerSpeed set to %d seconds\n", bannerSpeed);
     saveSettings();
    server.send(200, "text/plain", "OK"); });

            // ESP32 WebServer endpoint for setting labels without saving
            server.on("/setlabel", HTTP_POST, []()
                      {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON parse error");
        return;
    }

    String target = doc["target"];
    String value = doc["value"];

    if (target == "localTimeLabel") {
 localTimeLabel = "  " + value + "  ";
        Serial.printf("🕒 Updated localTimeLabel: %s\n", localTimeLabel.c_str());
    } else if (target == "utcTimeLabel") {
        utcTimeLabel = "  " + value + "  ";;
        Serial.printf("🌐 Updated utcTimeLabel: %s\n", utcTimeLabel.c_str());
    } else {
        server.send(400, "text/plain", "Unknown target");
        return;
    }

    // Redraw labels immediately on screen
    refreshFrames = true;
    drawOrredrawStaticElements();

    server.send(200, "text/plain", "OK"); });

            server.on("/setposition", HTTP_POST, []()
                      {


    String body = server.arg("plain");
    Serial.println("📩 Received JSON:");
    Serial.println(body);

    JsonDocument doc;  // enough for two numbers
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        Serial.println("❌ JSON parse error");
        server.send(400, "text/plain", "JSON parse error");
        return;
    }

    // parse as string -> float
    latitude  = String(doc["latitude"].as<const char*>()).toFloat();
    longitude = String(doc["longitude"].as<const char*>()).toFloat();

    Serial.printf("📍 Latitude updated to: %.6f\n", latitude);
    Serial.printf("📍 Longitude updated to: %.6f\n", longitude);
    saveSettings();
    fetchWeatherData();
    server.send(200, "text/plain", "OK"); });

            server.on("/setitalic", HTTP_POST, []()
                      {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON parse error");
        return;
    }
    italicClockFonts = doc["italicClockFonts"] | italicClockFonts;
    saveSettings();
    Serial.printf("✏️ italicClockFonts set to: %s\n", italicClockFonts ? "true" : "false");

    drawOrredrawStaticElements();

    server.send(200, "text/plain", "OK"); });

            server.on("/saveall", HTTP_POST, []()
                      {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "❌ Missing JSON body");
        Serial.println("❌ No JSON payload received!");
        return;
    }

    String json = server.arg("plain");
    Serial.println("\n📨 Received JSON from webpage:");
    Serial.println(json);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    // 🔧 Apply settings directly to global variables (not config struct!)
    latitude             = doc["latitude"] | latitude;
    longitude            = doc["longitude"] | longitude;
    localTimeLabel       = doc["localTimeLabel"] | localTimeLabel;
    utcTimeLabel         = doc["utcTimeLabel"] | utcTimeLabel;
    italicClockFonts     = doc["italicClockFonts"] | italicClockFonts;
    doubleFrame          = doc["doubleFrame"] | doubleFrame;
    bannerSpeed          = doc["bannerSpeed"] | bannerSpeed;
    screenSaverTimeout   = doc["screenSaverTimeout"] | screenSaverTimeout;

    // 📋 Debug printout of applied values
    Serial.println("📋 Parsed and applied config:");
    Serial.println("──────────────────────────────────────────────");
    Serial.printf("📍 Latitude             : %.6f\n", latitude);
    Serial.printf("📍 Longitude            : %.6f\n", longitude);
    Serial.printf("🕒 Local Time Label     : %s\n", localTimeLabel.c_str());
    Serial.printf("🕒 UTC Time Label       : %s\n", utcTimeLabel.c_str());
    Serial.printf("✍️  Italic Fonts         : %s\n", italicClockFonts ? "true" : "false");
    Serial.printf("🖼️  Double Frame         : %s\n", doubleFrame ? "true" : "false");
    Serial.printf("🏃 Banner Speed         : %d\n", bannerSpeed);
    Serial.printf("💤 ScreenSaver Timeout  : %lu ms (%.2f min)\n",
                  screenSaverTimeout,
                  screenSaverTimeout / 60000.0);
    Serial.println("──────────────────────────────────────────────");

    // 💾 Save settings to SPIFFS (your version will do the actual work)
    saveSettings();
    Serial.println("✅ Settings saved to flash.");

    server.send(200, "text/plain", "💾 Settings saved to flash");
    esp_restart(); });

            server.on("/setbootimage", HTTP_POST, []()
                      {
                  if (!server.hasArg("plain"))
                  {
                      server.send(400, "text/plain", "Missing body");
                      return;
                  }

                  JsonDocument doc;
                  DeserializationError error = deserializeJson(doc, server.arg("plain"));
                  if (error)
                  {
                      server.send(400, "text/plain", "JSON parse error");
                      return;
                  }

          if (!doc["bootImageId"].is<const char*>()) {
    server.send(400, "text/plain", "Missing bootImageId");
    return;
}

                  startupLogo = doc["bootImageId"].as<String>();
                  Serial.printf("🖼️ Boot logo updated to: %s\n", startupLogo.c_str());
                  saveSettings(); // 💾 Persist the change

                  server.send(200, "text/plain", "Boot logo saved");
                esp_restart(); });

            server.on("/setbootimage", HTTP_POST, []()
                      {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "JSON parse error");
        return;
    }

   if (!doc["bootImageId"].is<const char*>()) {
    server.send(400, "text/plain", "Missing bootImageId");
    return;
}

    startupLogo = doc["bootImageId"].as<String>();
    Serial.printf("🖼️ Boot logo updated to: %s\n", startupLogo.c_str());

    saveSettings(); // 💾 Persist the change

    server.send(200, "text/plain", "Boot logo saved"); });

            server.on("/ping", HTTP_GET, []()
                      { server.send(200, "text/plain", "pong"); });

            server.on("/scrolltext", HTTP_GET, []()
                      { server.send(200, "text/plain", scrollText); });
            server.on("/uploadpng", HTTP_POST, []()
                      {
                          // ✅ no early response here
                      },
                      handlePNGUpload);

            server.on("/setScreenSaverTime", HTTP_POST, []()
                      {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    JsonDocument doc;
    if (!deserializeJson(doc, body)) {
      screenSaverTimeout = doc["screenSaverTimeout"] | 0;
      saveSettings();
      Serial.printf("🖥️ Screen Saver Timeout set to %lu ms (%lu minutes)\n",
                    screenSaverTimeout, screenSaverTimeout / 60000);
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"status\":\"bad request\"}"); });

            server.on("/apikey.html", handleApiKeyPage);

            server.on("/saveApiKey", handleSaveApiKey);
            server.on("/getApiKey", handleGetApiKey);

            server.on("/setAutoPage", HTTP_GET, []()
                      {
                          if (server.hasArg("enabled"))
                          {
                              autoPageChange = (server.arg("enabled") == "true");
                          }
                          server.send(200, "text/plain", autoPageChange ? "AutoPage ON" : "AutoPage OFF");
                          saveSettings();

                          activePage = 1;
                          drawOrredrawStaticElements(); // 🖼️ Redraw Big Clock frames
                      });

            server.begin();

            // Initialize NTP Client
            timeClient.begin();

            // Wait until time is valid
            while (!timeClient.update())
            {
                delay(500);
            }
            tryToRetrieveUTCoffsetFromFirstConfiguration();

            fetchWeatherData();
            fetchSolarData();
            drawOrredrawStaticElements();

            scrollingText.setColorDepth(8);
            scrollingText.createSprite(320, 30);      // Create a 310x20 sprite to accommodate the text width
            scrollingText.setTextColor(bannerColour); // White text
            scrollingText.setTextDatum(TL_DATUM);     // Top-left alignment for text

            // Set the font for the sprite
            scrollingText.setFreeFont(&Orbitron_Medium10pt7b);

            // Calculate the initial position (rightmost position)
            scrollingTextXposition = scrollingText.width();
        }
    }
}

void loop()
{
    server.handleClient();
    if (inAPmode == true)
    {
        dnsServer.processNextRequest(); // <- IMPORTANT
        return;
    }

    unsigned long currentMillis = millis();
    static unsigned long previousMillisForWeatherDataUpdate = 0;
    static unsigned long previousMillisForPropagationDataUpdate = 0;
    static unsigned long previousMillisForLargeClockUpdate = 0;
    static unsigned long previousMillisForPropagationClockUpdate = 0;
    static unsigned long previousMillisForWiFiPageUpdate = 0;
    static unsigned long previousMillisForScroller = 0;
    static unsigned long previousMillisForAutoPageChanger = 0;
    static unsigned long previousMillisForTimeClientUpdate = 0;
    static unsigned long previousMillisForBlinkDotsOnBigClock = 0;

    static unsigned long lastDotUpdate = 0;
    static bool screenSaver = false;
    if (!screenSaver)
    {
        handleTouchToRotatePage();
    }

    // 🌤️ Refresh weather data every 5 minutes
    if (currentMillis - previousMillisForWeatherDataUpdate >= 1000UL * 60 * 5 && APIkeyIsValid)
    {
        previousMillisForWeatherDataUpdate = currentMillis;
        Serial.println("Getting fresh weather data");

        fetchWeatherData();
    }
    // 🌤️ Refresh propagation data every 5 minutes
    if (currentMillis - previousMillisForPropagationDataUpdate >= 1000UL * 60 * 5)
    {
        previousMillisForPropagationDataUpdate = currentMillis;
        Serial.println("Getting fresh propagation data");
        fetchSolarData();
        redrawMainPropagationPage = true;
        redrawSolarSummaryPage1 = true;
        redrawSolarSummaryPage2 = true;
        redrawSolarSummaryPage3 = true;
    }

    // Check for inactivity → Enable screensaver
    if (!screenSaver && currentMillis - lastActivity > screenSaverTimeout)
    {
        if (screenSaverTimeout != 120)
        {
            screenSaver = true;
            Serial.println("⏳ Inactivity detected — entering screensaver.");
        }
    }

    // 💤 Screensaver Mode
    if (screenSaver)
    {
        // 🌈 Refresh random pixel animation every 1 second
        if (currentMillis - lastDotUpdate >= 1000)
        {
            tft.fillScreen(TFT_BLACK);

            for (int i = 0; i < 200; i++)
            {
                int x = random(0, 320);
                int y = random(0, 240);
                uint16_t color = tft.color565(random(256), random(256), random(256));
                tft.drawPixel(x, y, color);
            }

            lastDotUpdate = currentMillis;
        }
        uint16_t x, y;
        if (tft.getTouch(&x, &y))
        {
            Serial.println("🖐 Touch detected — exiting screensaver.");
            screenSaver = false;
            tft.fillScreen(TFT_BLACK);
            activePage = 1;
            Serial.println("📄 Active page -> 1");
            drawOrredrawStaticElements(); // 🖼️ Redraw Big Clock frames
            lastActivity = currentMillis; // 🔄 Reset inactivity timer
        }
        return;
    }
    // 📺 Normal Mode
    else
        switch (activePage)
        {
        case 1:
            if (currentMillis - previousMillisForLargeClockUpdate >= 1000) // to not overflow
            {
                previousMillisForLargeClockUpdate = currentMillis;
                UTClastTimeStr = "        ";
                LOCALlastTimeStr = "        ";

                // 🕒 Update time display
                timeClient.update();
                long localEpoch = timeClient.getEpochTime() + (tOffset * 3600);
                String localTime = formatLocalTime(localEpoch);
                String utcTime = timeClient.getFormattedTime();

                tft.setTextColor(TFT_WHITE);
                tft.setFreeFont(italicClockFonts ? &digital_7_monoitalic42pt7b : &digital_7__mono_42pt7b);
                displayTime(8, 5, localTime, previousLocalTime, 0, localTimeColour);
                displayTime(10, 107, utcTime, previousUTCtime, 0, utcTimeColour);
            }
            // 📰 Scroll banner text
            if (currentMillis - previousMillisForScroller >= bannerSpeed)
            {
                previousMillisForScroller = currentMillis;
                scrollingText.fillSprite(TFT_BLACK);
                scrollingText.setTextColor(bannerColour);
                scrollingText.drawString(scrollText, scrollingTextXposition, 0);
                scrollingTextXposition -= 1;
                if (scrollingTextXposition < -scrollingText.textWidth(scrollText))
                    scrollingTextXposition = scrollingText.width();
                scrollingText.pushSprite(5, 205);
            }
            break;

        case 2:

            if (redrawMainPropagationPage == true)
            {
                Serial.println("Displaying Main Propagation Page");
                drawMainPropagationPage();
                redrawMainPropagationPage = false;
                LOCALlastTimeStr = "        "; // 8 characters: HH:MM:SS
                UTClastTimeStr = "        ";   // 8 characters: HH:MM:SS
            }

            if (currentMillis - previousMillisForPropagationClockUpdate >= 1000) // to not overflow
            {
                previousMillisForPropagationClockUpdate = currentMillis;

                colonVisible = !colonVisible;
                timeClient.update();
                long localEpoch = timeClient.getEpochTime() + (tOffset * 3600);
                String localTime = formatLocalTime(localEpoch);
                String utcTime = timeClient.getFormattedTime();

                drawLOCALTime(String(localTime), 30, 205, LOCALdigitColor, TFT_BLACK, blinkingDot);
                drawUTCTime(String(utcTime), 30 + 160, 205, UTCdigitColor, TFT_BLACK, blinkingDot);
            }
            break;

        case 3:
            if (redrawSolarSummaryPage1 == true)
            {
                Serial.println("Displaying Propagation Page 2");
                redrawSolarSummaryPage1 = false;
                drawSolarSummaryPage1();
            }

            break;

        case 4:
            if (redrawSolarSummaryPage2 == true)
            {
                Serial.println("Displaying Propagation Page 2");
                redrawSolarSummaryPage2 = false;
                drawSolarSummaryPage2();
            }

            break;

        case 5:
            if (redrawSolarSummaryPage3 == true)
            {
                Serial.println("Displaying Propagation Page 2");
                redrawSolarSummaryPage3 = false;
                drawSolarSummaryPage3();
            }

            break;
        case 6:
            if (reDrawWiFiQualityPage == true)
            {
                Serial.println("Displaying Wifi Quality Page");
                reDrawWiFiQualityPage = false;

                drawWiFiQualityPage();
            }
            if (currentMillis - previousMillisForWiFiPageUpdate >= 1000) // to not overflow
            {
                updateWiFiSignalDisplay();
                previousMillisForWiFiPageUpdate = currentMillis;
            }
            break;
        case 7:
            currentMillis = millis();
            tft.setFreeFont(&digits60pt7b);

            if (currentMillis - previousMillisForTimeClientUpdate >= 16000 || LASTbigClockTimeStr == "") // to not overflow
            {
                // 🕒 Update time display
                if (timeClient.update())
                {
                    Serial.println("NTP update OK");
                    successFullTimeUpdate = true;
                    tft.setTextColor(bigClockColour);
                    tft.drawString(":", 151, 65, 1);
                }
                else
                {
                    Serial.println("NTP update FAILED");
                    successFullTimeUpdate = false;
                    tft.setTextColor(TFT_RED);
                    tft.drawString(":", 151, 65, 1);
                }
                previousMillisForTimeClientUpdate = currentMillis;
            }
            long localEpoch = timeClient.getEpochTime() + (tOffset * 3600);

            struct tm *ptm = gmtime((time_t *)&localEpoch);

            String localTime = String(ptm->tm_hour < 10 ? "0" : "") + String(ptm->tm_hour) + ":" +
                               String(ptm->tm_min < 10 ? "0" : "") + String(ptm->tm_min);

            // Positions for HH:MM digits
            const int digitX[4] = {5, 78, 180, 253}; // x positions for H1, H2, M1, M2
            const int digitY = 65;                   // same Y for all digits

            if (localTime != LASTbigClockTimeStr)
            {
                // Break current time "HH:MM" into 4 chars
                char currentDigits[4];
                currentDigits[0] = localTime.charAt(0); // H tens
                currentDigits[1] = localTime.charAt(1); // H ones
                currentDigits[2] = localTime.charAt(3); // M tens
                currentDigits[3] = localTime.charAt(4); // M ones

                // Compare digit by digit
                for (int i = 0; i < 4; i++)
                {
                    if (currentDigits[i] != bigClockLastDigit[i])
                    {
                        // Erase old digit
                        tft.setTextColor(TFT_BLACK);
                        tft.drawString(String(bigClockLastDigit[i]), digitX[i], digitY, 1);

                        // Draw new digit
                        tft.setTextColor(bigClockColour);
                        tft.drawString(String(currentDigits[i]), digitX[i], digitY, 1);

                        // Update bigClockLastDigit
                        bigClockLastDigit[i] = currentDigits[i];
                    }
                }
                LASTbigClockTimeStr = localTime;
            }
            break;
        }

    if (autoPageChange)
    {
        // Serial.println();
        if (currentMillis - previousMillisForAutoPageChanger >= 1000 * 15)
        {
            previousMillisForAutoPageChanger = currentMillis;
            if (activePage == 1)
            {
                activePage = 2;
                redrawMainPropagationPage = true;
                return;
            }
            if (activePage == 2)
            {
                activePage = 1;
                drawOrredrawStaticElements(); // 🖼️ Redraw Big Clock frames
            }
        }
    }
}

// Fetch weather data
void fetchWeatherData()
{
    HTTPClient http;
    String weatherURL = weatherAPI + "?lat=" + String(latitude) + "&lon=" + String(longitude) + "&appid=" + apiKey + "&units=metric";

    // Make GET Request
    Serial.println("");
    Serial.println("Weather API URL to test in Broswer");
    Serial.println(weatherURL);
    Serial.println("");

    http.begin(weatherURL);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        // Serial.println(payload);
        Serial.println("Weather data received.");

        // Parse the JSON response
        JsonDocument doc;
        deserializeJson(doc, payload);

        // Extracting values from the JSON response and assigning to variables

        // Coordinates
        float lon = doc["coord"]["lon"];
        float lat = doc["coord"]["lat"];

        // Weather
        int weatherId = doc["weather"][0]["id"];
        const char *weatherMain = doc["weather"][0]["main"];
        const char *weatherDescription = doc["weather"][0]["description"];
        const char *weatherIcon = doc["weather"][0]["icon"];

        // Base
        const char *base = doc["base"];

        // Main weather data
        float temp = doc["main"]["temp"];
        float feels_like = doc["main"]["feels_like"];
        float temp_min = doc["main"]["temp_min"];
        float temp_max = doc["main"]["temp_max"];
        int pressure = doc["main"]["pressure"];
        int humidity = doc["main"]["humidity"];
        int sea_level = doc["main"]["sea_level"];
        int grnd_level = doc["main"]["grnd_level"];

        // Visibility
        int visibility = doc["visibility"];

        // Wind data
        float wind_speed = doc["wind"]["speed"];
        int wind_deg = doc["wind"]["deg"];
        float wind_gust = doc["wind"]["gust"];

        // Rain data
        float rain_1h = doc["rain"]["1h"];

        // Clouds data
        int clouds_all = doc["clouds"]["all"];

        // Date/Time
        long dt = doc["dt"];

        // System data
        int sys_type = doc["sys"]["type"];
        int sys_id = doc["sys"]["id"];
        const char *sys_country = doc["sys"]["country"];
        long sunrise = doc["sys"]["sunrise"];
        long sunset = doc["sys"]["sunset"];

        // Timezone
        int tzOffsetSeconds = 0;
        if (doc["timezone"].is<int>())
        {
            tzOffsetSeconds = doc["timezone"].as<int>(); // e.g. 7200
        }
        int tzOffsetHours = tzOffsetSeconds / 3600;
        tOffset = tzOffsetHours;
        Serial.printf("🕰️ tOffset set to %d (from %d seconds retrieved from API response)\n", tOffset, tzOffsetSeconds);

        // Location data
        int id = doc["id"];
        const char *name = doc["name"];

        // Status code
        int cod = doc["cod"];

        // Print the extracted values
        Serial.println("Weather data received.");
        Serial.print("Coordinates: ");
        Serial.print("Longitude: ");
        Serial.print(lon);
        Serial.print(", Latitude: ");
        Serial.println(lat);

        Serial.print("Weather ID: ");
        Serial.println(weatherId);
        Serial.print("Main: ");
        Serial.println(weatherMain);
        Serial.print("Description: ");
        Serial.println(weatherDescription);
        Serial.print("Icon: ");
        Serial.println(weatherIcon);

        Serial.print("Base: ");
        Serial.println(base);

        Serial.print("Temperature: ");
        Serial.println(temp);
        Serial.print("Feels like: ");
        Serial.println(feels_like);
        Serial.print("Min Temp: ");
        Serial.println(temp_min);
        Serial.print("Max Temp: ");
        Serial.println(temp_max);
        Serial.print("Pressure: ");
        Serial.println(pressure);
        Serial.print("Humidity: ");
        Serial.println(humidity);
        Serial.print("Sea level: ");
        Serial.println(sea_level);
        Serial.print("Ground level: ");
        Serial.println(grnd_level);

        Serial.print("Visibility: ");
        Serial.println(visibility);

        Serial.print("Wind speed: ");
        Serial.println(wind_speed);
        Serial.print("Wind degree: ");
        Serial.println(wind_deg);
        Serial.print("Wind gust: ");
        Serial.println(wind_gust);

        Serial.print("Rain 1h: ");
        Serial.println(rain_1h);

        Serial.print("Clouds: ");
        Serial.println(clouds_all);

        Serial.print("Timestamp: ");
        Serial.println(dt);

        Serial.print("System type: ");
        Serial.println(sys_type);
        Serial.print("System ID: ");
        Serial.println(sys_id);
        Serial.print("Country: ");
        Serial.println(sys_country);
        Serial.print("Sunrise: ");
        Serial.println(sunrise);
        Serial.print("Sunset: ");
        Serial.println(sunset);

        Serial.print("Timezone (sec): ");
        Serial.println(tzOffsetSeconds);

        Serial.print("Location ID: ");
        Serial.println(id);
        Serial.print("Location Name: ");
        Serial.println(name);

        Serial.print("Status code: ");
        Serial.println(cod);

        // Convert sunrise and sunset times to local time
        long localSunrise = sunrise + (tOffset * 3600); // Adjust for local time (seconds)
        long localSunset = sunset + (tOffset * 3600);   // Adjust for local time (seconds)

        // Convert sunrise and sunset times to human-readable format
        String sunriseTime = convertEpochToTimeString(localSunrise);
        String sunsetTime = convertEpochToTimeString(localSunset);
        String date = convertTimestampToDate(dt); // Convert to DD:MM:YY format
        // Build the scrollText with the date, weather, sunrise, and sunset times
        scrollText = String(name) + "     " + sys_country + "    " +
                     date + "     " +
                     "Tmp: " + String(temp, 1) + " C     " + // One decimal place for temp
                     "RH: " + String(humidity) + "%" + "       " +
                     "Pres: " + String(pressure) + "hPa" + "       " +
                     String(weatherDescription) + "       " +
                     "Sunrise: " + sunriseTime + "     " +
                     "Sunset: " + sunsetTime;

        scrollingText.drawString(scrollText, scrollingTextXposition, 0); // Draw text in sprite at position scrollingTextXposition
        scrollingTextXposition = scrollingText.width();
        Serial.println(scrollText);
        APIkeyIsValid = true;
    }
    else
    {
        Serial.print("Error fetching weather data, HTTP code: ");
        Serial.println(httpCode);
        scrollText = "Sorry, No Weather Info At This Moment!!!            Have you enterred your API key?"; // Text to scroll
        scrollingTextXposition = scrollingText.width();
        APIkeyIsValid = false;
    }

    http.end();
}

// Function to format the local time from epoch time
String formatLocalTime(long epochTime)
{
    struct tm *timeInfo;
    timeInfo = localtime(&epochTime); // Convert epoch to local time
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo); // Format time as HH:MM:SS
    return String(buffer);
}

// Function to convert an epoch time to a human-readable time string
String convertEpochToTimeString(long epochTime)
{
    struct tm *timeInfo;
    timeInfo = localtime(&epochTime); // Convert epoch to local time
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo); // Format time as HH:MM:SS
    return String(buffer);
}

void displayTime(int x, int y, String time, String &previousTime, int yOffset, uint16_t fontColor)
{
    if (refreshDigits)
    {
        refreshDigitsCounter++;
        if (refreshDigitsCounter == 1)
        {
            // First call — force clear by using empty string
            previousTime = "";
        }
        else if (refreshDigitsCounter >= 2)
        {
            // Second call — stop refreshing
            previousTime = "";
            refreshDigits = false;
            refreshDigitsCounter = 0;
        }
    }

    // Define the calculated positions for each character
    int positions[] = {x, x + 48, x + 78, x + 108, x + 156, x + 186, x + 216, x + 264};

    // Loop over the time string and compare it with the previous time
    for (int i = 0; i < time.length(); i++)
    {
        if (time[i] != previousTime[i])
        {
            tft.setTextColor(TFT_BLACK);
            tft.drawString(String(previousTime[i]), positions[i], y + yOffset, 1);
            tft.setTextColor(fontColor);
            tft.drawString(String(time[i]), positions[i], y + yOffset, 1);
        }
    }

    previousTime = time;
}

void *fileOpen(const char *filename, int32_t *size)
{
    String fullPath = "/" + String(filename);
    pngFile = SPIFFS.open(fullPath, "r");
    if (!pngFile)
        return nullptr;
    *size = pngFile.size();
    return (void *)&pngFile;
}

void fileClose(void *handle)
{
    ((fs::File *)handle)->close();
}

int32_t fileRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
    return ((fs::File *)handle->fHandle)->read(buffer, length);
}

int32_t fileSeek(PNGFILE *handle, int32_t position)
{
    return ((fs::File *)handle->fHandle)->seek(position);
}

void displayPNGfromSPIFFS(const char *filename, int duration_ms)
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("Failed to mount SPIFFS!");
        return;
    }

    int16_t rc = png.open(filename, fileOpen, fileClose, fileRead, fileSeek, [](PNGDRAW *pDraw)
                          {
    uint16_t lineBuffer[480];  // Adjust to your screen width if needed
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
    tft.pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer); });

    if (rc == PNG_SUCCESS)
    {
        Serial.printf("Displaying PNG: %s\n", filename);
        tft.startWrite();
        png.decode(nullptr, 0);
        tft.endWrite();
    }
    else
    {
        Serial.println("PNG decode failed.");
    }

    delay(duration_ms);
}

String convertTimestampToDate(long timestamp)
{
    struct tm *timeinfo;
    timeinfo = localtime(&timestamp);                       // Convert epoch to local time
    char buffer[11];                                        // Buffer for "DD:MM:YY"
    strftime(buffer, sizeof(buffer), "%d:%m:%y", timeinfo); // Format as DD:MM:YY
    return String(buffer);
}

void loadSettings()
{

    fs::File file = SPIFFS.open("/settings.json", "r");
    if (!file)
    {
        Serial.println("⚠️ Could not open settings file. Using defaults.");
        saveSettings();
        esp_restart();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("⚠️ Failed to parse settings file. Using defaults.");
        file.close();
        return;
    }

    file.close(); // Always close file after use

    latitude = doc["latitude"] | latitude;
    longitude = doc["longitude"] | longitude;
    localTimeColour = doc["localTimeColour"] | localTimeColour;
    utcTimeColour = doc["utcTimeColour"] | utcTimeColour;
    doubleFrame = doc["doubleFrame"] | doubleFrame;
    localFrameColour = doc["localFrameColour"] | localFrameColour;
    utcFrameColour = doc["utcFrameColour"] | utcFrameColour;
    bannerColour = doc["bannerColour"] | bannerColour;
    bannerSpeed = doc["bannerSpeed"] | bannerSpeed;
    localTimeLabel = doc["localTimeLabel"] | localTimeLabel;
    utcTimeLabel = doc["utcTimeLabel"] | utcTimeLabel;
    startupLogo = doc["startupLogo"] | startupLogo;
    italicClockFonts = doc["italicClockFonts"] | italicClockFonts;
    screenSaverTimeout = doc["screenSaverTimeout"] | screenSaverTimeout;
    autoPageChange = doc["autoPageChange"] | autoPageChange;
    useScreenSaver = doc["useScreenSaver"] | useScreenSaver;
    bigClockColour = doc["bigClockColour"] | bigClockColour;

    Serial.println();
    Serial.println("-----------------------------------------------------------------");
    Serial.println("✅ Settings loaded from SPIFFS:");
    Serial.printf("📍 latitude: %.6f\n", latitude);
    Serial.printf("📍 longitude: %.6f\n", longitude);
    Serial.printf("🎨 localTimeColour: 0x%04X\n", localTimeColour);
    Serial.printf("🎨 utcTimeColour: 0x%04X\n", utcTimeColour);
    Serial.printf("🌀 doubleFrame: %s\n", doubleFrame ? "true" : "false");
    Serial.printf("🎨 localFrameColour: 0x%04X\n", localFrameColour);
    Serial.printf("🎨 utcFrameColour: 0x%04X\n", utcFrameColour);
    Serial.printf("🖍️ bannerColour: 0x%04X\n", bannerColour);
    Serial.printf("🖍️ bigClockColour: 0x%04X\n", bigClockColour);

    Serial.printf("🐢 bannerSpeed: %d\n", bannerSpeed);
    Serial.printf("🕓 localTimeLabel: %s\n", localTimeLabel.c_str());
    Serial.printf("🌍 utcTimeLabel: %s\n", utcTimeLabel.c_str());
    Serial.printf("🖼️ startupLogo: %s\n", startupLogo.c_str());
    Serial.printf("🔤 italicClockFonts: %s\n", italicClockFonts ? "true" : "false");
    Serial.printf("🕓 screenSaverTimeout: %lu ms\n", screenSaverTimeout);
    Serial.printf("⚡ Auto Page Change    : %s\n", autoPageChange ? "true" : "false");
    Serial.printf("⚡ Use Screen Saver    : %s\n", useScreenSaver ? "true" : "false");
    Serial.println("-----------------------------------------------------------------");
}

void saveSettings()
{
    JsonDocument doc;
    doc["latitude"] = latitude;
    doc["longitude"] = longitude;
    doc["localTimeColour"] = localTimeColour;
    doc["utcTimeColour"] = utcTimeColour;
    doc["doubleFrame"] = doubleFrame;
    doc["localFrameColour"] = localFrameColour;
    doc["utcFrameColour"] = utcFrameColour;
    doc["bannerColour"] = bannerColour;
    doc["bannerSpeed"] = bannerSpeed;
    doc["localTimeLabel"] = localTimeLabel;
    doc["utcTimeLabel"] = utcTimeLabel;
    doc["startupLogo"] = startupLogo;
    doc["italicClockFonts"] = italicClockFonts;
    doc["autoPageChange"] = autoPageChange;
    doc["useScreenSaver"] = useScreenSaver;
    doc["bigClockColour"] = bigClockColour;

    fs::File file = SPIFFS.open("/settings.json", "w");

    if (!file)
    {
        Serial.println("❌ Failed to open settings file for writing");
        return;
    }

    serializeJsonPretty(doc, file);
    file.close();

    // ✅ Nicely formatted output
    Serial.println("");
    Serial.println(F("────────────────────────────────────────"));

    Serial.println("💾 Saving settings to SPIFFS:");
    Serial.println(F("────────────────────────────────────────"));
    Serial.printf("🌍 Latitude           : %f\n", latitude);
    Serial.printf("🌍 Longitude          : %f\n", longitude);
    Serial.printf("🕒 Local Time Color   : 0x%04X\n", localTimeColour);
    Serial.printf("🕒 UTC Time Color     : 0x%04X\n", utcTimeColour);
    Serial.printf("🕒 Big Time Color     : 0x%04X\n", bigClockColour);
    Serial.printf("🖼️  Double Frame      : %s\n", doubleFrame ? "true" : "false");
    Serial.printf("🟩 Local Frame Color  : 0x%04X\n", localFrameColour);
    Serial.printf("🟦 UTC Frame Color    : 0x%04X\n", utcFrameColour);
    Serial.printf("📜 Banner Color       : 0x%04X\n", bannerColour);
    Serial.printf("⚡ Banner Speed       : %d\n", bannerSpeed);
    Serial.printf("🔤 Local Time Label   : %s\n", localTimeLabel.c_str());
    Serial.printf("🔤 UTC Time Label     : %s\n", utcTimeLabel.c_str());
    Serial.printf("🖼️  Startup Logo      : %s\n", startupLogo.c_str());
    Serial.printf("✏️  Italic Fonts      : %s\n", italicClockFonts ? "true" : "false");
    Serial.printf("😴 Screensaver (ms)   : %lu\n", screenSaverTimeout);
    Serial.printf("⚡ Use screensaver    : %s\n", useScreenSaver ? "true" : "false");
    Serial.printf("⚡ Auto Page Change    : %s\n", autoPageChange ? "true" : "false");

    Serial.println(F("────────────────────────────────────────"));

    Serial.println("✅ Settings saved to SPIFFS");
    Serial.println("");
}

void handleRoot()
{
    fs::File file = SPIFFS.open("/index.html", "r"); // ✅ Declare 'file' properly here
    if (!file)
    {
        server.send(500, "text/plain", "⚠️ Failed to open index.html");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}
void handleApiKeyPage()
{
    File file = SPIFFS.open("/apikey.html", "r");
    if (!file)
    {
        server.send(404, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleSave()
{
    if (server.hasArg("latitude"))
        latitude = server.arg("latitude").toFloat();
    if (server.hasArg("longitude"))
        longitude = server.arg("longitude").toFloat();
    if (server.hasArg("bannerSpeed"))
        bannerSpeed = server.arg("bannerSpeed").toInt();
    if (server.hasArg("localLabel"))
        localTimeLabel = server.arg("localLabel");
    if (server.hasArg("utcLabel"))
        utcTimeLabel = server.arg("utcLabel");
    if (server.hasArg("logo"))
        startupLogo = server.arg("logo");
    if (server.hasArg("italicFont"))
        italicClockFonts = (server.arg("italicFont") == "on");

    saveSettings(); // Save updated settings

    server.send(200, "text/html", "<h1>✅ Settings saved!</h1><a href='/'>Back</a>");
}

void drawOrredrawStaticElementsOLD()
{
    // Only run if we want to refresh the frames
    if (refreshFrames)
    {
        refreshFramesCounter++;
        if (refreshFramesCounter < 2)
        {
            return; // Wait for second execution
        }
        refreshFrames = false;
        refreshFramesCounter = 0;
    }
    previousLocalTime = "";
    previousUTCtime = "";
    tft.setFreeFont(&Orbitron_Medium8pt7b);
    tft.fillRect(25, 0 + 85 - 10, 270, 20, TFT_BLACK);
    tft.fillRect(25, 106 + 85 - 10, 270, 20, TFT_BLACK);

    // 🟩 Local Frame
    tft.fillRect(0, 0, 320, 87, TFT_BLACK); // Clear previous frame
    tft.drawRoundRect(1, 1, 318, 85, 4, TFT_BLACK);

    tft.drawRoundRect(0, 0, 320, 87, 5, localFrameColour);
    if (doubleFrame)
    {
        tft.drawRoundRect(1, 1, 318, 85, 4, localFrameColour);
        tft.drawRoundRect(2, 2, 316, 83, 4, localFrameColour);
        tft.drawRoundRect(3, 3, 314, 81, 4, localFrameColour);
    }

    // 🟦 Local Time Label

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawCentreString(localTimeLabel, 160, 76, 1);

    // 🟥 UTC Frame
    tft.fillRect(0, 105, 320, 87, TFT_BLACK); // Clear previous frame
    tft.drawRoundRect(1, 106, 318, 85, 4, TFT_BLACK);

    tft.drawRoundRect(0, 105, 320, 87, 5, utcFrameColour);
    if (doubleFrame)
    {
        tft.drawRoundRect(1, 106, 318, 85, 4, utcFrameColour);
        tft.drawRoundRect(2, 107, 316, 83, 4, utcFrameColour);
        tft.drawRoundRect(3, 108, 314, 81, 4, utcFrameColour);
    }

    // ⬜ UTC Label
    tft.drawCentreString(utcTimeLabel, 160, 76 + 105, 1);
}

void drawOrredrawStaticElements()
{
    // Only run if we want to refresh the frames
    tft.fillScreen(TFT_BLACK);
    if (refreshFrames)
    {
        refreshFramesCounter++;
        if (refreshFramesCounter < 2)
        {
            return; // Wait for second execution
        }
        refreshFrames = false;
        refreshFramesCounter = 0;
    }
    previousLocalTime = "";
    previousUTCtime = "";
    tft.setFreeFont(&Orbitron_Medium8pt7b);
    tft.fillRect(25, 0 + 85 - 10, 270, 20, TFT_BLACK);
    tft.fillRect(25, 106 + 85 - 10, 270, 20, TFT_BLACK);

    // 🟩 Local Frame
    tft.fillRect(0, 0, 320, 87, TFT_BLACK); // Clear previous frame

    tft.drawRoundRect(1, 1, 319, 85, 5, localFrameColour);
    if (doubleFrame)
    {
        tft.drawRoundRect(1, 1, 319, 85, 4, localFrameColour);
        tft.drawRoundRect(2, 2, 317, 83, 4, localFrameColour);
        tft.drawRoundRect(3, 3, 315, 81, 4, localFrameColour);
    }

    // 🟦 Local Time Label

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawCentreString(localTimeLabel, 160, 76, 1);

    // 🟥 UTC Frame
    tft.fillRect(0, 105, 320, 87, TFT_BLACK); // Clear previous frame

    tft.drawRoundRect(1, 105, 319, 85, 5, utcFrameColour);
    if (doubleFrame)
    {
        tft.drawRoundRect(1, 106, 319, 85, 4, utcFrameColour);
        tft.drawRoundRect(2, 107, 317, 83, 4, utcFrameColour);
        tft.drawRoundRect(3, 108, 315, 81, 4, utcFrameColour);
    }

    // ⬜ UTC Label
    tft.drawCentreString(utcTimeLabel, 160, 76 + 105, 1);
}

void mountAndListSPIFFS(uint8_t levels, bool listContent)
{
    Serial.println();
    if (!SPIFFS.begin(true))
    {
        Serial.println("\n❌ Failed to mount SPIFFS.");
        return;
    }
    Serial.println("\n✅ SPIFFS mounted successfully!");

    if (!listContent)
        return;

    Serial.println("📂 Listing SPIFFS content:");
    fs::File root = SPIFFS.open("/");
    if (!root || !root.isDirectory())
    {
        Serial.println("❌ Failed to open root directory or not a directory.");
        return;
    }

    fs::File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  📁 DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                String path = String("/") + file.name();
                mountAndListSPIFFS(levels - 1, true); // Recursive listing
            }
        }
        else
        {
            Serial.print("  📄 FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }

    // Calculate and print free space information
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    size_t free = total - used;
    float percentFree = ((float)free / total) * 100.0;

    Serial.println();
    Serial.println("📊 SPIFFS Usage Info:");
    Serial.printf("   📦 Total: %u bytes\n", total);
    Serial.printf("   📂 Used : %u bytes\n", used);
    Serial.printf("   📭 Free : %u bytes (%.2f%%)\n", free, percentFree);
    Serial.println();
}

void handlePNGUpload()
{
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.printf("📁 Uploading PNG: %s\n", upload.filename.c_str());

        // 🖥️ Blank screen and show "Receiving" + "New" + "Splash Screen" on 3 lines
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setFreeFont(&Orbitron_Light_32);
        tft.drawCentreString("Receiving", 160, 10, 1);

        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString("New", 160, 60, 1);
        tft.drawCentreString("Splash Screen", 160, 110, 1);

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.setFreeFont(&Orbitron_Medium8pt7b);
        tft.drawCentreString("Please wait...", 160, 170, 1);

        fs::File file = SPIFFS.open("/logo4.png", FILE_WRITE);
        if (!file)
        {
            Serial.println("❌ Failed to open file for writing");
            return;
        }
        file.close();
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        fs::File file = SPIFFS.open("/logo4.png", FILE_APPEND);
        if (file)
        {
            file.write(upload.buf, upload.currentSize);
            file.close();
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        Serial.printf("✅ Upload complete: %s (%d bytes)\n", upload.filename.c_str(), upload.totalSize);
        server.send(200, "text/plain", "✅ PNG upload complete. Will be used at next boot.");
        startupLogo = "logo4.png";
        displayPNGfromSPIFFS("logo4.png", 3000);
        saveSettings();
        tft.fillScreen(TFT_BLACK);
        drawOrredrawStaticElements();
    }
}

void handleTouchToRotatePage()
{
    uint16_t x, y;
    bool touching = tft.getTouch(&x, &y);
    unsigned long now = millis();

    if (touching)
    {
        if (!wasTouching && (now - lastTouchMs > 250)) // edge detect + debounce
        {
            wasTouching = true;
            lastTouchMs = now;

            // rotate page
            activePage = (activePage % MAX_PAGES) + 1;
            Serial.printf("📄 Active page -> %u\n", activePage);
            tft.fillScreen(TFT_BLACK);
            if (activePage == 1)
            {
                drawOrredrawStaticElements(); // 🖼️ Redraw Big Clock frames
            }
            if (activePage == 2)
            {
                redrawMainPropagationPage = true;
            }
            if (activePage == 3)
            {
                redrawSolarSummaryPage1 = true;
            }
            if (activePage == 4)
            {
                redrawSolarSummaryPage2 = true;
            }
            if (activePage == 5)
            {
                redrawSolarSummaryPage3 = true;
            }
            if (activePage == 6)
            {
                reDrawWiFiQualityPage = true;
            }
            if (activePage == 7)
            {
                LASTbigClockTimeStr = "";
                for (int i = 0; i < 4; i++) {
    bigClockLastDigit[i] = ' '; 
}   
                tft.fillScreen(TFT_BLACK);
            }
        }
    }
    else
    {
        wasTouching = false;
    }
}

void drawMainPropagationPage()
{

    tft.fillScreen(TFT_BLACK);
    // draw frames
    //  Define positions and dimensions
    int dayX = 10;
    int nightX = 170;
    int blockY = 12;
    int blockWidth = 140;
    int blockHeight = 143;
    int cornerRadius = 8;

    tft.drawRoundRect(dayX, blockY, blockWidth, blockHeight, cornerRadius, TFT_DARKGREY);
    tft.drawRoundRect(nightX, blockY, blockWidth, blockHeight, cornerRadius, TFT_DARKGREY);
    tft.fillRect(80 - 27, 0, 54, 20, TFT_BLACK);
    tft.fillRect(240 - 38, 0, 76, 20, TFT_BLACK);

    // Draw headers
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawCentreString("DAY", 80, 2, 1);
    tft.drawCentreString("NIGHT", 240, 2, 1);

    // Band conditions by time
    tft.setFreeFont(&JetBrainsMono_Bold15pt7b);

    int yStart = 22;
    for (int i = 0; i < 4; i++)
    {
        // DAY

        String band = solarData.bandConditions[i].name;
        String cond = solarData.bandConditions[i].condition;
        uint16_t color = cond == "Good" ? TFT_GREEN : cond == "Fair" ? TFT_YELLOW
                                                                     : TFT_RED;
        tft.setTextColor(color);
        tft.drawCentreString(band, 80, yStart + i * 32, 1);

        // NIGHT
        band = solarData.bandConditions[i + 4].name;
        cond = solarData.bandConditions[i + 4].condition;
        color = cond == "Good" ? TFT_GREEN : cond == "Fair" ? TFT_YELLOW
                                                            : TFT_RED;
        tft.setTextColor(color);

        tft.drawCentreString(band, 240, yStart + i * 32, 1);
    }

    tft.setFreeFont(&JetBrainsMono_Light7pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawCentreString("Updated: " + solarData.updated, 160, 160, 1);
    // tft.drawCentreString("Updating...", 80, 185, 1);
    // tft.drawCentreString("Updating...", 240, 185, 1);

    int LocalX = 10;
    int UTCX = 170;
    blockY = 190;
    blockWidth = 140;
    blockHeight = 48;
    cornerRadius = 8;

    tft.drawRoundRect(LocalX, blockY, blockWidth, blockHeight, cornerRadius, TFT_DARKGREY);
    tft.drawRoundRect(UTCX, blockY, blockWidth, blockHeight, cornerRadius, TFT_DARKGREY);
    tft.fillRect(80 - 36, blockY - 15, 72, 35, TFT_BLACK);
    tft.fillRect(240 - 26, blockY - 15, 52, 35, TFT_BLACK);

    // Draw headers
    tft.setFreeFont(&JetBrainsMono_Bold11pt7b);
    tft.setTextColor(TFT_LIGHTGREY);

    tft.drawCentreString("Local", 80, 179, 1);
    tft.drawCentreString("UTC", 240, 179, 1);
}

void fetchSolarData()
{

    // Fetch XML
    HTTPClient http;
    http.begin(solarDataUrl);
    int httpCode = http.GET();

    if (httpCode <= 0)
    {
        Serial.println("HTTP request failed");
        return;
    }

    String payload = http.getString();
    tinyxml2::XMLDocument doc;
    doc.Parse(payload.c_str());
    if (doc.ErrorID() != 0)
    {
        Serial.print("XML parse error: ");
        Serial.println(doc.ErrorStr());
        return;
    }

    tinyxml2::XMLElement *solardataXML = doc.RootElement()->FirstChildElement("solardata");

    auto get = [&](const char *tag)
    {
        tinyxml2::XMLElement *e = solardataXML->FirstChildElement(tag);
        return e && e->GetText() ? String(e->GetText()) : String("");
    };

    // Assign fields to struct
    solarData.source = get("source");
    // solarData.updated = get("updated"); reformatted because like this 31 Jul 2025 1321 GMT
    solarData.updated = formatUpdatedTimestampToUTC(get("updated"));
    solarData.solarFlux = get("solarflux").toInt();
    solarData.aIndex = get("aindex").toInt();
    solarData.kIndex = get("kindex").toInt();
    solarData.kIndexNT = get("kindexnt");
    solarData.xRay = get("xray");
    solarData.sunspots = get("sunspots").toInt();
    solarData.heliumLine = get("heliumline").toFloat();
    solarData.protonFlux = get("protonflux");
    solarData.electronFlux = get("electonflux");
    solarData.aurora = get("aurora").toInt();
    solarData.normalization = get("normalization").toFloat();
    solarData.latDegree = get("latdegree").toFloat();
    solarData.solarWind = get("solarwind").toFloat();
    solarData.magneticField = get("magneticfield").toFloat();
    solarData.geomagneticField = get("geomagfield");
    solarData.signalNoise = get("signalnoise");
    solarData.fof2 = get("fof2");
    solarData.mufFactor = get("muffactor");
    solarData.muf = get("muf");

    // Parse band conditions
    int bIndex = 0;
    tinyxml2::XMLElement *band = solardataXML->FirstChildElement("calculatedconditions")->FirstChildElement("band");
    while (band && bIndex < 8)
    {
        solarData.bandConditions[bIndex].name = band->Attribute("name");
        solarData.bandConditions[bIndex].time = band->Attribute("time");
        solarData.bandConditions[bIndex].condition = band->GetText();
        band = band->NextSiblingElement("band");
        bIndex++;
    }

    // Parse VHF conditions
    int vIndex = 0;
    tinyxml2::XMLElement *phen = solardataXML->FirstChildElement("calculatedvhfconditions")->FirstChildElement("phenomenon");
    while (phen && vIndex < 5)
    {
        solarData.vhfConditions[vIndex].name = phen->Attribute("name");
        solarData.vhfConditions[vIndex].location = phen->Attribute("location");
        solarData.vhfConditions[vIndex].condition = phen->GetText();
        phen = phen->NextSiblingElement("phenomenon");
        vIndex++;
    }

    http.end();

    // --- Serial Debug Output ---
    Serial.println("\n=== Solar Data ===");
    Serial.println("Source: " + solarData.source);
    Serial.println("Updated: " + solarData.updated);
    Serial.printf("Solar Flux: %d\n", solarData.solarFlux);
    Serial.printf("A Index: %d\n", solarData.aIndex);
    Serial.printf("K Index: %d\n", solarData.kIndex);
    Serial.println("K Index NT: " + solarData.kIndexNT);
    Serial.println("X-Ray: " + solarData.xRay);
    Serial.printf("Sunspots: %d\n", solarData.sunspots);
    Serial.printf("Helium Line: %.1f\n", solarData.heliumLine);
    Serial.println("Proton Flux: " + solarData.protonFlux);
    Serial.println("Electron Flux: " + solarData.electronFlux);
    Serial.printf("Aurora: %d\n", solarData.aurora);
    Serial.printf("Normalization: %.2f\n", solarData.normalization);
    Serial.printf("Lat Degree: %.2f\n", solarData.latDegree);
    Serial.printf("Solar Wind: %.1f\n", solarData.solarWind);
    Serial.printf("Magnetic Field: %.1f\n", solarData.magneticField);
    Serial.println("Geomagnetic Field: " + solarData.geomagneticField);
    Serial.println("Signal Noise: " + solarData.signalNoise);
    Serial.println("foF2: " + solarData.fof2);
    Serial.println("MUF Factor: " + solarData.mufFactor);
    Serial.println("MUF: " + solarData.muf);

    Serial.println("--- Band Conditions ---");
    for (int i = 0; i < 8; i++)
    {
        if (solarData.bandConditions[i].name.isEmpty())
            break;
        Serial.printf("[%s] %s: %s\n",
                      solarData.bandConditions[i].time.c_str(),
                      solarData.bandConditions[i].name.c_str(),
                      solarData.bandConditions[i].condition.c_str());
    }

    Serial.println("--- VHF Conditions ---");
    for (int i = 0; i < 5; i++)
    {
        if (solarData.vhfConditions[i].name.isEmpty())
            break;
        Serial.printf("%s (%s): %s\n",
                      solarData.vhfConditions[i].name.c_str(),
                      solarData.vhfConditions[i].location.c_str(),
                      solarData.vhfConditions[i].condition.c_str());
    }
}

String formatUpdatedTimestampToUTC(const String &raw)
{
    int gmtPos = raw.indexOf("GMT");
    if (gmtPos == -1 || gmtPos < 5)
        return raw; // malformed or too short

    // Extract 4 characters before "GMT" → should be the time
    String timePart = raw.substring(gmtPos - 5, gmtPos - 1); // e.g. "1321"
    if (timePart.length() != 4)
        return raw;

    // Insert colon in the time
    String formattedTime = timePart.substring(0, 2) + ":" + timePart.substring(2, 4);

    // Everything before the time
    String datePart = raw.substring(0, gmtPos - 5);
    datePart.trim(); // remove any leading/trailing whitespace

    return datePart + " " + formattedTime + " UTC";
}

void drawLOCALTime(const String &timeStr, int x, int y, uint16_t digitColor, uint16_t backgroundColor, bool blinkColon)
{
    tft.setFreeFont(&HB97DIGITS12pt7b);

    for (int i = 0; i < 8; i++)
    {
        char newChar = timeStr.charAt(i);
        char oldChar = LOCALlastTimeStr.charAt(i);
        int xpos = x + xOffsets[i];

        // Always redraw colon, toggling its color
        if (i == 2 || i == 5)
        {
            uint16_t colonColor = blinkColon ? (colonVisible ? digitColor : backgroundColor) : digitColor;
            tft.setTextColor(colonColor, backgroundColor);
            tft.drawString(":", xpos, y, 1);
            continue; // skip rest of loop for colon
        }

        // Redraw only if digit changed
        if (newChar != oldChar)
        {
            // Erase old character
            tft.setTextColor(backgroundColor, backgroundColor);
            tft.drawString(String(oldChar), xpos, y, 1);

            // Draw new character
            tft.setTextColor(digitColor, backgroundColor);
            tft.drawString(String(newChar), xpos, y, 1);
        }
    }

    // Save current drawn string (colons not modified)
    LOCALlastTimeStr = timeStr;
}

void drawUTCTime(const String &timeStr, int x, int y, uint16_t digitColor, uint16_t backgroundColor, bool blinkColon)
{
    tft.setFreeFont(&HB97DIGITS12pt7b);

    for (int i = 0; i < 8; i++)
    {
        char newChar = timeStr.charAt(i);
        char oldChar = UTClastTimeStr.charAt(i);
        int xpos = x + xOffsets[i];

        // Always redraw colon, toggling its color
        if (i == 2 || i == 5)
        {
            uint16_t colonColor = blinkColon ? (colonVisible ? digitColor : backgroundColor) : digitColor;
            tft.setTextColor(colonColor, backgroundColor);
            tft.drawString(":", xpos, y, 1);
            continue; // skip rest of loop for colon
        }

        // Redraw only if digit changed
        if (newChar != oldChar)
        {
            // Erase old character
            tft.setTextColor(backgroundColor, backgroundColor);
            tft.drawString(String(oldChar), xpos, y, 1);

            // Draw new character
            tft.setTextColor(digitColor, backgroundColor);
            tft.drawString(String(newChar), xpos, y, 1);
        }
    }

    // Save current drawn string (colons not modified)
    UTClastTimeStr = timeStr;
}

void drawSolarSummaryPage1()
{
    int y = 13;
    int lineSpacing = 18;
    tft.fillScreen(TFT_BLACK);

    tft.setFreeFont(&UbuntuMono_Regular8pt7b);
    tft.setTextSize(1);

    // Adjust spacing
    const int labelX = 10;
    const int valueX = 120;
    const int commentX = 200;

    auto printLine = [&](const String &label, const String &value, uint16_t color, const String &comment = "")
    {
        tft.setTextColor(color, TFT_BLACK); // foreground on black
        tft.setCursor(labelX, y);
        tft.print(label);
        tft.setCursor(valueX, y);
        tft.print(": ");
        tft.print(value);
        if (comment.length() > 0)
        {
            tft.setCursor(commentX, y);
            tft.print("(" + comment + ")");
        }
        y += lineSpacing;
    };

    // Color + comment logic
    auto kIndexColorComment = [](int k)
    {
        if (k >= 7)
            return std::make_pair(TFT_RED, "Severe");
        if (k >= 5)
            return std::make_pair(TFT_RED, "Storm Risk");
        if (k >= 4)
            return std::make_pair(TFT_ORANGE, "Unsettled");
        if (k >= 2)
            return std::make_pair(TFT_YELLOW, "Quiet");
        return std::make_pair(TFT_GREEN, "Very Quiet");
    };

    auto aIndexColorComment = [](int a)
    {
        if (a >= 30)
            return std::make_pair(TFT_RED, "Disturbed");
        if (a >= 20)
            return std::make_pair(TFT_ORANGE, "Unsettled");
        if (a >= 10)
            return std::make_pair(TFT_YELLOW, "Normal");
        return std::make_pair(TFT_GREEN, "Quiet");
    };

    auto solarFluxColorComment = [](int sfi)
    {
        if (sfi >= 150)
            return std::make_pair(TFT_GREEN, "Excellent");
        if (sfi >= 100)
            return std::make_pair(TFT_YELLOW, "Good");
        return std::make_pair(TFT_RED, "Poor");
    };

    auto xrayColorComment = [](const String &x)
    {
        if (x.startsWith("X"))
            return std::make_pair(TFT_RED, "Extreme");
        if (x.startsWith("M"))
            return std::make_pair(TFT_ORANGE, "Moderate");
        if (x.startsWith("C"))
            return std::make_pair(TFT_YELLOW, "Low");
        return std::make_pair(TFT_GREEN, "Quiet");
    };

    auto [sfColor, sfComment] = solarFluxColorComment(solarData.solarFlux);
    printLine("Solar Flux", String(solarData.solarFlux), sfColor, sfComment);

    auto [aColor, aComment] = aIndexColorComment(solarData.aIndex);
    printLine("A Index", String(solarData.aIndex), aColor, aComment);

    auto [kColor, kComment] = kIndexColorComment(solarData.kIndex);
    printLine("K Index", String(solarData.kIndex), kColor, kComment);

    printLine("K Index NT", solarData.kIndexNT, TFT_WHITE);

    auto [xrColor, xrComment] = xrayColorComment(solarData.xRay);
    printLine("X-Ray", solarData.xRay, xrColor, xrComment);

    printLine("Sunspots", String(solarData.sunspots), TFT_WHITE);
    printLine("Helium Line", String(solarData.heliumLine, 1), TFT_WHITE);
    printLine("Proton Flux", solarData.protonFlux, TFT_WHITE);
    printLine("Electron Flux", solarData.electronFlux, TFT_WHITE);
    printLine("Aurora", String(solarData.aurora), TFT_WHITE);
    printLine("Normalization", String(solarData.normalization, 2), TFT_WHITE);
    printLine("Lat Degree", String(solarData.latDegree, 2), TFT_WHITE);
    printLine("Solar Wind", String(solarData.solarWind, 1), TFT_WHITE);
}

void drawSolarSummaryPage2()
{
    int y = 13;
    int lineSpacing = 18;
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&UbuntuMono_Regular8pt7b);
    tft.setTextSize(1);

    const int labelX = 10;
    const int valueX = 120;
    const int commentX = 200;

    auto printLine = [&](const String &label, const String &value, uint16_t color = TFT_WHITE, const String &comment = "")
    {
        tft.setTextColor(color, TFT_BLACK);
        tft.setCursor(labelX, y);
        tft.print(label);
        tft.setCursor(valueX, y);
        tft.print(": ");
        tft.print(value);
        if (comment.length() > 0)
        {
            tft.setCursor(commentX, y);
            tft.print("(" + comment + ")");
        }
        y += lineSpacing;
    };

    // Color and comment logic for text conditions
    auto conditionColorComment = [](const String &cond)
    {
        if (cond.equalsIgnoreCase("Good"))
            return std::make_pair(TFT_GREEN, "Good");
        if (cond.equalsIgnoreCase("Fair"))
            return std::make_pair(TFT_YELLOW, "Fair");
        if (cond.equalsIgnoreCase("Poor"))
            return std::make_pair(TFT_RED, "Poor");
        if (cond.indexOf("Storm") >= 0)
            return std::make_pair(TFT_RED, "Storm");
        if (cond.indexOf("Unsettled") >= 0)
            return std::make_pair(TFT_ORANGE, "Unsettled");
        return std::make_pair(TFT_WHITE, "");
    };

    printLine("Mag Field", String(solarData.magneticField, 1), TFT_WHITE);

    auto [geoColor, geoComment] = conditionColorComment(solarData.geomagneticField);
    printLine("Geo Field", solarData.geomagneticField, geoColor, geoComment);

    auto [snrColor, snrComment] = conditionColorComment(solarData.signalNoise);
    printLine("S/N", solarData.signalNoise, snrColor, snrComment);

    printLine("foF2", solarData.fof2, TFT_WHITE);
    printLine("MUF Fact", solarData.mufFactor, TFT_WHITE);
    printLine("MUF", solarData.muf, TFT_WHITE);
}

void drawSolarSummaryPage3()
{
    int y = 20;
    int lineSpacing = 18;
    int paragraphSpacing = 6;

    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&UbuntuMono_Regular8pt7b);
    tft.setTextSize(1);

    const int titleX = 10;
    const int resultX = 20;

    auto beautifyLocation = [](const String &raw) -> String
    {
        if (raw == "europe")
            return "Europe";
        if (raw == "north_america")
            return "North America";
        if (raw == "northern_hemi")
            return "Northern Hemisphere";
        if (raw == "europe_6m")
            return "Europe 6m";
        if (raw == "europe_4m")
            return "Europe 4m";
        return raw;
    };

    auto annotatePhenomenon = [](const String &name) -> String
    {
        if (name.equalsIgnoreCase("E-Skip"))
            return "E-Skip (Sporadic-E)";
        return name;
    };

    auto vhfColorComment = [](const String &val) -> std::pair<uint16_t, String>
    {
        if (val.equalsIgnoreCase("Band Open"))
            return {TFT_GREEN, "Excellent"};
        if (val.equalsIgnoreCase("Band Weak"))
            return {TFT_YELLOW, "Marginal"};
        if (val.equalsIgnoreCase("Band Closed"))
            return {TFT_RED, "No Propagation"};
        if (val.indexOf("ES") >= 0)
            return {TFT_GREEN, "Sporadic-E Active"};
        return {TFT_WHITE, ""};
    };

    auto printLine = [&](const String &title, const String &value, uint16_t color = TFT_WHITE, const String &comment = "")
    {
        tft.setTextColor(TFT_WHITE, TFT_BLACK); // title line always white
        tft.setCursor(titleX, y);
        tft.print(title);
        y += lineSpacing;

        tft.setTextColor(color, TFT_BLACK);
        tft.setCursor(resultX, y);
        tft.print(value);
        if (!comment.isEmpty())
        {
            tft.print("   (" + comment + ")");
        }
        y += lineSpacing + paragraphSpacing;
    };

    for (int i = 0; i < 5; i++)
    {
        if (solarData.vhfConditions[i].name.isEmpty())
            break;

        String name = annotatePhenomenon(solarData.vhfConditions[i].name);
        String location = beautifyLocation(solarData.vhfConditions[i].location);
        String condition = solarData.vhfConditions[i].condition;

        auto [color, comment] = vhfColorComment(condition);
        String title = name + " (" + location + ")";

        printLine(title, condition, color, comment);
    }
}

void updateWiFiSignalDisplay()
{
    int rssi = WiFi.RSSI();
    int quality = constrain(2 * (rssi + 100), 0, 100);

    // Static variables to track previous values
    static String lastRSSI = "";
    static String lastSignal = "";

    // Convert new values to strings
    String newRSSI = String(rssi) + " dBm";
    String newSignal = String(quality) + "%";

    // Coordinates based on drawWiFiQualityPage
    int rssiX = 130;
    int rssiY = 15 + 3 * 18;
    int signalX = 130;
    int signalY = 15 + 4 * 18;

    tft.setTextColor(TFT_BLACK, TFT_BLACK); // erase with background color

    // Erase previous RSSI
    tft.setCursor(rssiX, rssiY);
    tft.print(": ");
    tft.print(lastRSSI);

    // Erase previous Signal %
    tft.setCursor(signalX, signalY);
    tft.print(": ");
    tft.print(lastSignal);

    // Draw updated values
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.setCursor(rssiX, rssiY);
    tft.print(": ");
    tft.print(newRSSI);

    tft.setCursor(signalX, signalY);
    tft.print(": ");
    tft.print(newSignal);

    // Update bar graph
    drawWiFiSignalMeter(quality);

    // Save current values for next comparison
    lastRSSI = newRSSI;
    lastSignal = newSignal;
}

void drawWiFiSignalMeter(int qualityPercent)
{
    const int meterX = 18;
    const int meterY = 200;
    const int barWidth = 24;
    const int barSpacing = 5;
    const int barHeight = 18;
    const int numBars = 10;

    // Map quality (0–100%) to number of bars (0–10)
    int activeBars = map(qualityPercent, 0, 100, 0, numBars);

    for (int i = 0; i < numBars; i++)
    {
        int x = meterX + i * (barWidth + barSpacing);
        uint16_t color = TFT_DARKGREY;

        if (i < activeBars)
        {
            if (qualityPercent <= 30)
                color = TFT_RED;
            else if (qualityPercent <= 70)
                color = TFT_YELLOW;
            else
                color = TFT_GREEN;
        }

        tft.fillRect(x, meterY, barWidth, barHeight, color);
    }
    // draw a border around the full meter
    tft.drawRect(meterX - 2, meterY - 2, numBars * (barWidth + barSpacing) - barSpacing + 4, barHeight + 4, TFT_LIGHTGREY);
}

void drawWiFiQualityPage()
{
    tft.fillScreen(TFT_BLACK);
    // tft.setFreeFont(&UbuntuMono_Regular8pt7b);
    tft.setFreeFont(&FreeSans9pt7b);

    tft.setTextSize(1);

    int y = 15;
    const int lineSpacing = 18;

    auto printLine = [&](const String &label, const String &value, uint16_t color = TFT_WHITE)
    {
        tft.setTextColor(color, TFT_BLACK);
        tft.setCursor(10, y);
        tft.print(label);
        tft.setCursor(130, y);
        tft.print(": ");
        tft.print(value);
        y += lineSpacing;
    };

    String ssid = WiFi.SSID();
    String ip = WiFi.localIP().toString();
    String mac = WiFi.macAddress();
    int rssi = WiFi.RSSI();
    int quality = constrain(2 * (rssi + 100), 0, 100);
    String gateway = WiFi.gatewayIP().toString();
    String subnet = WiFi.subnetMask().toString();
    String dns = WiFi.dnsIP().toString();
    String hostname = WiFi.getHostname();

    printLine(" SSID", ssid);
    printLine(" IP", ip);
    printLine(" MAC", mac);
    printLine(" RSSI", String(rssi) + " dBm");
    printLine(" Signal", String(quality) + "%");
    printLine(" Gateway", gateway);
    printLine(" Subnet", subnet);
    printLine(" DNS", dns);
    printLine(" Hostname 1", hostname + ".local");
    printLine(" Hostname 2", "hamclock.local");

    drawWiFiSignalMeter(quality);
}
void drawQRCode(const char *text, int x, int y, int scale)
{
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text);

    for (uint8_t row = 0; row < qrcode.size; row++)
    {
        for (uint8_t col = 0; col < qrcode.size; col++)
        {
            int color = qrcode_getModule(&qrcode, col, row) ? TFT_BLACK : TFT_WHITE;
            tft.fillRect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}
void drawQRcodeInstructions()
{
    ;
    // Draw QR instructions
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("Wi-Fi Configuration", 160, 10, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawCentreString("1", 80, 38, 4);
    tft.drawCentreString("2", 160 + 80, 38, 4);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.drawCentreString("Scan to Join", 80, 85, 2);
    drawQRCode("WIFI:T:nopass;S:HB9IIUSetup;;", 80 - 116 / 2, 105, 4);

    tft.drawCentreString("Open config page", 240, 85, 2);
    drawQRCode("http://192.168.4.1", 240 - 116 / 2, 105, 4);
}

void handleRootCaptivePortal()
{
    server.send_P(200, "text/html", index_html);
}

void handleScanCaptivePortal()
{
    Serial.println("Returning scan list");
    String json = "[";
    for (int i = 0; i < scanCount; i++)
    {
        if (i > 0)
            json += ",";
        json += "\"" + WiFi.SSID(i) + "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleSaveCaptivePortal()
{
    Serial.println("Saving");

    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("time"))
    {
        String ssid = server.arg("ssid");
        String pass = server.arg("password");
        String timeStr = server.arg("time"); // JSON string: {"localTime":"15:42","offset":120}

        // --- Save WiFi ---
        prefs.begin("wifi", false);
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        prefs.end();

        // --- Parse JSON from "time" ---
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, timeStr);

        if (!err)
        {
            String isoTime = doc["iso"].as<String>();
            unsigned long long unixMillis = doc["unix"].as<unsigned long long>();
            int offsetMinutes = doc["offset"].as<int>();

            prefs.begin("iPhonetime", false);
            prefs.putString("iso", isoTime);
            prefs.putLong64("unix", unixMillis); // store as 64-bit
            prefs.putInt("offsetMinutes", offsetMinutes);
            prefs.end();

            Serial.printf("✅ Saved Phone Time:\n   ISO: %s\n   Unix: %llu\n   Offset: %d minutes\n",
                          isoTime.c_str(), unixMillis, offsetMinutes);
        }
        else
        {
            Serial.println("⚠️ Failed to parse time JSON, saving raw string instead.");
            prefs.begin("iPhonetime", false);
            prefs.putString("localTime", timeStr);
            prefs.end();
        }

        server.send_P(200, "text/html", html_success);
        ESP.restart();
    }
    else
    {
        server.send(400, "text/plain", "Missing fields.");
    }
}

void startConfigurationPortal()
{
    Serial.println("🌐 Starting Captive Portal...");
    drawQRcodeInstructions();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("HB9IIUSetup");
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    // DNS: redirect all domains to our AP IP
    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("📡 Scanning for networks...");
    WiFi.scanDelete();
    scanCount = WiFi.scanNetworks();
    Serial.printf("📶 Found %d networks\n", scanCount);

    // Routes
    server.on("/", handleRootCaptivePortal);
    server.on("/scan", handleScanCaptivePortal);
    server.on("/save", HTTP_POST, handleSaveCaptivePortal);
    server.onNotFound([]()
                      {
    Serial.print("Unknown request: ");
    Serial.println(server.uri());

    // Redirect everything to the root captive portal page
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", ""); });
    server.begin();
    Serial.println("🚀 Web server started.");
}

bool tryToConnectSavedWiFi()
{
    Serial.println("🔍 Attempting to load saved WiFi credentials...");

    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    prefs.end();

    if (ssid.isEmpty() || pass.isEmpty())
    {
        Serial.println("⚠️ No saved credentials found.");
        return false;
    }

    Serial.printf("📡 Found SSID: %s\n", ssid.c_str());
    Serial.printf("🔐 Found Password: %s\n", pass.c_str());

    Serial.printf("🔌 Connecting to WiFi: %s...\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());

    for (int i = 0; i < 40; i++) // wait up to ~20s
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("✅ Connected to WiFi!");
            Serial.print("📶 IP Address: ");
            Serial.println(WiFi.localIP());

            // 👉 Override DNS after DHCP has completed
            IPAddress dns2(8, 8, 8, 8);
            IPAddress dns1(1, 1, 1, 1);
            if (!WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2))
            {
                Serial.println("⚠️ Failed to set DNS servers.");
            }

            Serial.print("🌍 DNS #1: ");
            Serial.println(WiFi.dnsIP(0));
            Serial.print("🌍 DNS #2: ");
            Serial.println(WiFi.dnsIP(1));
            return true;
        }
        Serial.print(".");
        delay(500);
    }

    Serial.println("\n❌ Failed to connect to saved WiFi.");
    startConfigurationPortal();
    return false;
}

void checkIfscreenIsTouchedDuringStartUpForFactoryReset()
{
    // Wait up to 1 second to detect if screen touched
    bool screenWasTouched = false;
    uint32_t screenScanTimeStart = millis();
    while (millis() - screenScanTimeStart < 1000)
    {
        uint16_t x, y;
        if (tft.getTouch(&x, &y))
        {
            screenWasTouched = true;
            break;
        }
    }
    if (!screenWasTouched)
        return;

    // Local helpers encapsulated inside this function
    auto drawButton = [&](int x, int y, const char *label,
                          uint16_t bgColor, uint16_t textColor = TFT_WHITE)
    {
        tft.fillRoundRect(x, y, 120, 40, 6, bgColor);
        tft.drawRoundRect(x, y, 120, 40, 6, TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(textColor, bgColor);
        tft.drawString(label, x + 60, y + 20, 4);
    };

    auto isInsideButton = [&](int tx, int ty, int bx, int by)
    {
        return (tx >= bx && tx <= bx + 120 &&
                ty >= by && ty <= by + 40);
    };

    // Draw UI
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Factory Reset ?", tft.width() / 2, 35, 4);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("⚠ All settings will be lost", tft.width() / 2, 70, 2);
    tft.drawString("This action cannot be undone", tft.width() / 2, 100, 2);

    const int BTN_X = (tft.width() / 2 - 60);
    const int BTN_Y1 = 130;
    const int BTN_Y2 = BTN_Y1 + 40 + 20;

    drawButton(BTN_X, BTN_Y1, "Proceed", TFT_GREEN, TFT_BLACK);
    drawButton(BTN_X, BTN_Y2, "Exit", TFT_RED, TFT_WHITE);

    bool invertY = true; // set false if normal
    bool invertX = false;

    // Modal loop
    while (true)
    {
        uint16_t x, y;
        if (tft.getTouch(&x, &y))
        {
            if (invertX)
                x = tft.width() - x;
            if (invertY)
                y = tft.height() - y;

            if (isInsideButton(x, y, BTN_X, BTN_Y1))
            {
                Serial.println("Proceed pressed!");
                // --- Clear Preferences ---
                prefs.begin("wifi", false);
                prefs.clear();
                prefs.end();
                prefs.begin("config", false);
                prefs.clear();
                prefs.end();
                // --- Remove /settings.json from SPIFFS ---
                if (SPIFFS.begin(true)) // true = format if mount fails
                {
                    if (SPIFFS.exists("/settings.json"))
                    {
                        if (SPIFFS.remove("/settings.json"))
                        {
                            Serial.println("Deleted /settings.json successfully!");
                        }
                        else
                        {
                            Serial.println("Failed to delete /settings.json!");
                        }
                    }
                    else
                    {
                        Serial.println("/settings.json not found.");
                    }
                }
                else
                {
                    Serial.println("Failed to mount SPIFFS!");
                }
                // Restart ESP
                ESP.restart();
            }
            else if (isInsideButton(x, y, BTN_X, BTN_Y2))
            {
                Serial.println("Exit pressed!");
                // Restart ESP
                ESP.restart();
            }
            delay(300); // debounce
        }
    }
}
void handleSaveApiKey()
{
    if (server.hasArg("key"))
    {
        String apiKey = server.arg("key");
        prefs.begin("config", false);
        prefs.putString("ow_api_key", apiKey);
        prefs.end();
        server.send(200, "text/plain", "API key saved");
        fetchWeatherData();
    }
    else
    {
        server.send(400, "text/plain", "Missing key");
    }
}

void handleGetApiKey()
{
    prefs.begin("config", true);
    apiKey = prefs.getString("ow_api_key", "No API Key yet");
    prefs.end();
    server.send(200, "text/plain", apiKey);
}
void retrieveAPIkeyFromPref()
{
    prefs.begin("config", true);
    apiKey = prefs.getString("ow_api_key", "No API Key yet");
    prefs.end();
}

void tryToRetrieveUTCoffsetFromFirstConfiguration()
{
    prefs.begin("iPhonetime", true);

    String storedIso = prefs.getString("iso", "N/A");          // Full ISO timestamp
    unsigned long long setupUnix = prefs.getLong64("unix", 0); // ms since 1970
    int offsetMinutes = prefs.getInt("offsetMinutes", 0);

    prefs.end();

    // Debug output
    Serial.println("📂 Retrieved iPhonetime prefs:");
    Serial.printf("   ISO Timestamp: %s\n", storedIso.c_str());
    Serial.printf("   UTC Offset Minutes: %d (%.1f hours)\n",
                  offsetMinutes, offsetMinutes / 60.0);

    // ✅ Use NTPClient's clock, not system time
    unsigned long long nowUnix = timeClient.getEpochTime(); // seconds since 1970 (UTC)

    // Debug raw numbers
    Serial.printf("   Raw setupUnix (ms): %llu\n", setupUnix);
    Serial.printf("   Now (s): %llu\n", nowUnix);
    Serial.printf("   Setup (s): %llu\n", setupUnix / 1000ULL);

    // Calculate elapsed
    unsigned long long elapsed = nowUnix - (setupUnix / 1000ULL);

    unsigned long days = elapsed / 86400;
    unsigned long hours = (elapsed % 86400) / 3600;
    unsigned long minutes = (elapsed % 3600) / 60;
    unsigned long seconds = elapsed % 60;

    Serial.printf("⏱️ Configured %lu days, %lu hours, %lu minutes, %lu seconds ago\n",
                  days, hours, minutes, seconds);

    // Update Global variable
    tOffset = offsetMinutes / 60.0;
}