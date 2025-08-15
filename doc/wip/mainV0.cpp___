/*
 * Program Name: ESP32 Weather and Time Display
 * Author: HB9IIU
 * Created for: Marco T77PM (for testing and customizing)
 * Description:
 * This program displays the local and UTC time on a TFT screen and fetches weather data from the OpenWeather API.
 * The program includes a PNG logo display and uses custom fonts for the time display.
 * It connects to Wi-Fi and the NTP server for accurate time synchronization.
 *
 * Color List (TFT_eSPI):
 * --------------------------------
 * TFT_BLACK      - Black
 * TFT_WHITE      - White
 * TFT_BLUE       - Blue
 * TFT_RED        - Red
 * TFT_GREEN      - Green
 * TFT_CYAN       - Cyan
 * TFT_MAGENTA    - Magenta
 * TFT_YELLOW     - Yellow
 * TFT_ORANGE     - Orange
 * TFT_LIGHTGREY  - Light Grey
 * TFT_DARKGREY   - Dark Grey
 * TFT_BROWN      - Brown
 * TFT_PINK       - Pink
 * TFT_PURPLE     - Purple
 * TFT_GREY       - Grey
 * TFT_DARKGREEN  - Dark Green
 * TFT_LIGHTBLUE  - Light Blue
 * TFT_LIGHTGREEN - Light Green
 * TFT_LIGHTCYAN  - Light Cyan
 * TFT_LIGHTMAGENTA - Light Magenta
 *
 * Notes:
 * This code is based on the TFT_eSPI library and uses the NTPClient and HTTPClient libraries for fetching time
 * and weather data from external sources.
 * The program supports custom fonts and displays the information clearly on a TFT screen.
 */

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
#include <PNGdec.h>
#include <SPIFFS.h>
#include <config.h>


// Global variables for configuration
String SSID = WIFI_SSID; // Wi-Fi credentials
String WiFiPassword = WIFI_PASSWORD;
float latitude = LATITUDE;       // Latitude
float longitude = LONGITUDE;     // Longitude
String apiKey = WEATHER_API_KEY; // API Key

int tOffset = TIME_OFFSET;

const String weatherAPI = "https://api.openweathermap.org/data/2.5/weather"; // OpenWeather API endpoint

int retriesBeforeReboot = 5;

// Global variables for previous time tracking
String previousLocalTime = "";
String previousUTCtime = "";

// TFT Display Setup
TFT_eSPI tft = TFT_eSPI();              // Create TFT display object
TFT_eSprite stext2 = TFT_eSprite(&tft); // Sprite object for "Hello World" text

// Scrolling Text
int textX;                                                      // Variable for text position (to start at the rightmost side)
String scrollText = "Sorry, No Weather Info At This Moment!!! Have you enterred your API key?"; // Text to scroll
// Timing variables
unsigned long previousMillisForScroller = 0; // Store last time the action was performed

// NTP Client Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC offset and update interval

// WiFi Reconnect Logic
int retryCount = 0;

// Function Prototypes
void connectWiFi();
void fetchWeatherData();
String formatLocalTime(long epochTime);
String convertEpochToTimeString(long epochTime);
void displayTime(int x, int y, String time, String &previousTime, int yOffset, uint16_t fontColor);
String convertTimestampToDate(long timestamp);
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

    // Initialize TFT display
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    Serial.println("TFT Display initialized!");

    // Display PNG from SPIFFS
    displayPNGfromSPIFFS(START_UP_LOGO, 0);

    // Connect to Wi-Fi
    connectWiFi();

    // Initialize NTP Client
    timeClient.begin();
    timeClient.setTimeOffset(0); // UTC Offset (0 for UTC)
    Serial.println("NTP Client initialized.");
    tft.fillScreen(TFT_BLACK);

    fetchWeatherData();
    tft.setFreeFont(&Orbitron_Medium8pt7b);
    tft.drawRoundRect(0, 0, 320, 87, 5, LOCAL_FRAME);
    if (DOUBLE_FRAME)
    {
        tft.drawRoundRect(1, 1, 318, 85, 4, LOCAL_FRAME);
    }
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawCentreString(LOCAL_TIME_FRAME_LABEL, 160, 76, 1);
    tft.drawRoundRect(0, 105, 320, 87, 5, UTC_FRAME);
    if (DOUBLE_FRAME)
    {
        tft.drawRoundRect(1, 106, 318, 85, 4, UTC_FRAME);
    }
    tft.drawCentreString(UTC_TIME_FRAME_LABEL, 160, 76 + 105, 1);

    // Create a sprite for the Weather text
    stext2.setColorDepth(8);
    stext2.createSprite(310, 30);       // Create a 310x20 sprite to accommodate the text width
    stext2.setTextColor(BANNER_COLOUR); // White text
    stext2.setTextDatum(TL_DATUM);      // Top-left alignment for text

    // Set the font for the sprite
    stext2.setFreeFont(&Orbitron_Medium10pt7b); // Apply custom font to the sprite

    // Calculate the initial position (rightmost position)
    textX = stext2.width();
}

void loop()
{
    // Calculate time elapsed since last weather data fetch
    unsigned long currentMillis = millis();
    static unsigned long previousMillis = 0; // Store the last time the weather data was fetched

    // Update time every second
    timeClient.update();

    // Get Local Time by adding tOffset to UTC time
    long localEpoch = timeClient.getEpochTime() + (tOffset * 3600); // Add offset (in seconds)
    String localTime = formatLocalTime(localEpoch);                 // Format the local time

    // Get UTC Time
    String utcTime = timeClient.getFormattedTime();

    // Display Local and UTC Time with different y positions
    tft.setTextColor(TFT_WHITE); // Set text color to white
    if (ITALIC_CLOCK_FONTS)
    {
        tft.setFreeFont(&digital_7_monoitalic42pt7b);
    }
    else
    {
        tft.setFreeFont(&digital_7__mono_42pt7b);
    }
    // Corrected y positions for both clocks
    displayTime(8, 5, localTime, previousLocalTime, 0, LOCAL_TIME_COLOUR); // Display local time at y = 5
    displayTime(10, 107, utcTime, previousUTCtime, 0, UTC_TIME_COLOUR);    // Display UTC time at y = 106

    // Fetch Weather Data once every 5 minutes
    if (currentMillis - previousMillis >= 1000*60*5)
    {                                   
        previousMillis = currentMillis; // Save the current time
        fetchWeatherData();
    }
    // Check if the interval has passed
    if (currentMillis - previousMillisForScroller >= BANNER_SPEED)
    {
        // Save the last time the action was performed
        previousMillisForScroller = currentMillis;

        // Clear the sprite
        stext2.fillSprite(TFT_BLACK); // Fill sprite with background color

        // Draw the text inside the sprite at the specified position
        stext2.setTextColor(BANNER_COLOUR);
        stext2.drawString(scrollText, textX, 0); // Draw text in sprite at position `textX`

        // Scroll the text by shifting the position to the left
        textX -= 1; // Move text left by 1 pixel

        // Reset position when text has scrolled off the screen
        if (textX < -stext2.textWidth(scrollText))
        {                           // Text has completely scrolled off screen
            textX = stext2.width(); // Reset position to the far right
        }

        // Push the sprite onto the TFT at the specified coordinates
        stext2.pushSprite(5, 205); // Push the sprite to the screen at position (5, 220)
    }
  
}

// Function to connect to Wi-Fi
void connectWiFi()
{
    Serial.print("Connecting to Wi-Fi: ");
    Serial.print(SSID);
    WiFi.begin(SSID, WiFiPassword);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        retryCount++;
        if (retryCount >= retriesBeforeReboot)
        {
            Serial.println("Wi-Fi connection failed too many times, rebooting...");
            ESP.restart();
        }
    }

    Serial.println("Wi-Fi connected!");
}

// Fetch weather data
void fetchWeatherData()
{
    HTTPClient http;
    String weatherURL = weatherAPI + "?lat=" + String(latitude) + "&lon=" + String(longitude) + "&appid=" + apiKey + "&units=metric";

    // Make GET Request
    http.begin(weatherURL);
    Serial.println("");
    Serial.println(weatherURL);
    Serial.println("");

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("Weather data received.");

 
        // Parse the JSON response
        DynamicJsonDocument doc(1024);
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
        int timezone = doc["timezone"];

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

        Serial.print("Timezone: ");
        Serial.println(timezone);

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
                     "Temp: " + String(temp, 1) + "°C     " + // One decimal place for temp
                     "RH: " + String(humidity) + "%" + "       " +
                     String(weatherDescription) + "       " +
                     "Sunrise: " + sunriseTime + "     " +
                     "Sunset: " + sunsetTime;

        stext2.drawString(scrollText, textX, 0); // Draw text in sprite at position `textX`
        textX = stext2.width();
        Serial.println(scrollText);
    }
    else
    {
        Serial.print("Error fetching weather data, HTTP code: ");
        Serial.println(httpCode);
        scrollText = "Sorry, No Weather Info At This Moment!!!"; // Text to scroll
        textX = stext2.width();
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

// Function to display time (local or UTC) with change detection and custom font color
void displayTime(int x, int y, String time, String &previousTime, int yOffset, uint16_t fontColor)
{
    // Define the calculated positions for each character
    int positions[] = {x, x + 48, x + 78, x + 108, x + 156, x + 186, x + 216, x + 264};

    // Loop over the time string and compare it with the previous time
    for (int i = 0; i < time.length(); i++)
    {
        if (time[i] != previousTime[i])
        {                                // If the digit is different
            tft.setTextColor(TFT_BLACK); // Erase previous digit in black
            tft.drawString(String(previousTime[i]), positions[i], y + yOffset, 1);
            tft.setTextColor(fontColor);                                   // Use the passed font color for new digit
            tft.drawString(String(time[i]), positions[i], y + yOffset, 1); // Draw new digit
        }
    }

    // Update the previousTime to the current time after drawing
    previousTime = time;
}

// PNG Decoder Callback Functions
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

// Function to display PNG from SPIFFS
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

// Function to convert Unix timestamp to human-readable format (DD:MM:YY)
String convertTimestampToDate(long timestamp)
{
    struct tm *timeinfo;
    timeinfo = localtime(&timestamp);                       // Convert epoch to local time
    char buffer[11];                                        // Buffer for "DD:MM:YY"
    strftime(buffer, sizeof(buffer), "%d:%m:%y", timeinfo); // Format as DD:MM:YY
    return String(buffer);
}
