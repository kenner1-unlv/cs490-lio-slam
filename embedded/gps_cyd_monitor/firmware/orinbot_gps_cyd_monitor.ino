#include <Arduino.h>
#include <TinyGPS++.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

TinyGPSPlus gps;
TFT_eSPI tft = TFT_eSPI();
HardwareSerial GPSserial(1);
WiFiUDP udp;

// GPS UART pin that worked
#define GPS_RX 27

// Cheap Yellow Display backlight pin
#define TFT_BACKLIGHT 21

// WiFi
const char* WIFI_SSID = "QueenB";
const char* WIFI_PASS = "dynamicship";
const char* TZ_INFO = "PST8PDT,M3.2.0,M11.1.0";  // Pacific time with DST

// UDP broadcast. Orin listens on port 5005.
IPAddress UDP_TARGET_IP(255, 255, 255, 255);
const uint16_t UDP_TARGET_PORT = 5005;

// Timers
unsigned long lastDraw = 0;
unsigned long lastUdp = 0;
unsigned long lastWifiCheck = 0;

bool wifiConnected = false;

// Flip this if colors are reversed.
// true usually fixes the CYD white-background inversion problem.
const bool TFT_INVERT = true;

// -----------------------------
// Local/NTP time, GPS UTC fallback
// -----------------------------
void formatDisplayTime(char* buf, size_t len) {
  struct tm timeinfo;

  // Prefer internet/NTP local time
  if (getLocalTime(&timeinfo, 10)) {
    snprintf(buf, len, "%02d:%02d:%02d",
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    return;
  }

  // Fallback to GPS UTC if NTP/local time unavailable
  if (gps.time.isValid()) {
    snprintf(buf, len, "%02d:%02d:%02dZ",
             gps.time.hour(),
             gps.time.minute(),
             gps.time.second());
    return;
  }

  snprintf(buf, len, "--:--:--");
}

// -----------------------------
// WiFi boot screen
// -----------------------------
void drawWifiScreen(const char* status, const char* detail = "") {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 15);
  tft.println("ORINBOT GPS");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 55);
  tft.println("WiFi Setup");

  tft.setCursor(10, 95);
  tft.println(status);

  if (strlen(detail) > 0) {
    tft.setTextSize(1);
    tft.setCursor(10, 135);
    tft.println(detail);
  }
}

bool connectWifi() {
  drawWifiScreen("Connecting...", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");

  unsigned long start = millis();
  int dots = 0;

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillRect(10, 125, 220, 30, TFT_BLACK);
    tft.setCursor(10, 125);

    for (int i = 0; i < dots; i++) {
      tft.print(".");
    }

    dots = (dots + 1) % 8;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;

    String ip = WiFi.localIP().toString();

    drawWifiScreen("Connected", ip.c_str());

    Serial.print("WiFi connected. ESP32 IP: ");
    Serial.println(ip);

    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");

    Serial.println("Syncing local time from NTP...");

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      Serial.println("NTP time synced.");
    } else {
      Serial.println("NTP sync failed, GPS UTC fallback only.");
    }

    delay(1500);
    return true;
  } else {
    wifiConnected = false;

    drawWifiScreen("WiFi failed", "GPS display only");

    Serial.println("WiFi failed. Continuing GPS display only.");

    delay(1500);
    return false;
  }
}

// -----------------------------
// Static dashboard
// -----------------------------
void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("ORINBOT GPS");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(230, 10);
  tft.print("TIME");

  tft.drawFastHLine(10, 35, 300, TFT_DARKGREY);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);

  tft.setCursor(10, 45);
  tft.print("FIX");

  tft.setCursor(105, 45);
  tft.print("SATS");

  tft.setCursor(205, 45);
  tft.print("HDOP");

  tft.setCursor(10, 90);
  tft.print("LAT");

  tft.setCursor(10, 135);
  tft.print("LON");

  tft.setCursor(10, 180);
  tft.print("ALT");

  tft.setCursor(160, 180);
  tft.print("AGE");

  tft.setTextSize(1);
  tft.setCursor(10, 225);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    tft.print("IP ");
    tft.print(WiFi.localIP());
    tft.print(" UDP:");
    tft.print(UDP_TARGET_PORT);
  } else {
    tft.print("local GPS display mode");
  }
}

// -----------------------------
// Dynamic dashboard values only
// -----------------------------
void drawScreen() {
  tft.setTextSize(2);

  // Clear only value areas, not whole screen
  tft.fillRect(200, 10, 120, 22, TFT_BLACK);    // time
  tft.fillRect(10, 65, 80, 22, TFT_BLACK);      // fix
  tft.fillRect(105, 65, 80, 22, TFT_BLACK);     // sats
  tft.fillRect(205, 65, 100, 22, TFT_BLACK);    // hdop
  tft.fillRect(10, 110, 300, 24, TFT_BLACK);    // lat
  tft.fillRect(10, 155, 300, 24, TFT_BLACK);    // lon
  tft.fillRect(10, 200, 130, 22, TFT_BLACK);    // alt
  tft.fillRect(160, 200, 150, 22, TFT_BLACK);   // age

  // Local time from NTP, GPS UTC fallback
  char timeBuf[16];
  formatDisplayTime(timeBuf, sizeof(timeBuf));

  tft.setCursor(200, 10);

  struct tm tmpTime;
  if (getLocalTime(&tmpTime, 1) || gps.time.isValid()) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }

  tft.print(timeBuf);

  // Fix
  tft.setCursor(10, 65);
  if (gps.location.isValid()) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("YES");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("NO");
  }

  // Satellites
  tft.setCursor(105, 65);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (gps.satellites.isValid()) {
    tft.print(gps.satellites.value());
  } else {
    tft.print("--");
  }

  // HDOP
  tft.setCursor(205, 65);
  if (gps.hdop.isValid() && gps.hdop.hdop() < 1.0) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  }

  if (gps.hdop.isValid()) {
    tft.print(gps.hdop.hdop(), 2);
  } else {
    tft.print("--");
  }

  // Latitude
  tft.setCursor(10, 110);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (gps.location.isValid()) {
    tft.print(gps.location.lat(), 5);
  } else {
    tft.print("--.-----");
  }

  // Longitude
  tft.setCursor(10, 155);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (gps.location.isValid()) {
    tft.print(gps.location.lng(), 5);
  } else {
    tft.print("--.-----");
  }

  // Altitude
  tft.setCursor(10, 200);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  if (gps.altitude.isValid()) {
    tft.print(gps.altitude.meters(), 1);
    tft.print("m");
  } else {
    tft.print("--");
  }

  // Age
  tft.setCursor(160, 200);
  if (gps.location.isValid() && gps.location.age() < 1500) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }

  if (gps.location.isValid()) {
    tft.print(gps.location.age());
    tft.print("ms");
  } else {
    tft.print("--");
  }
}

// -----------------------------
// UDP converted GPS only
// -----------------------------
void sendGpsUdp() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!gps.location.isValid()) return;

  char timeBuf[16];
  formatDisplayTime(timeBuf, sizeof(timeBuf));

  char packet[256];

  snprintf(packet, sizeof(packet),
           "{\"lat\":%.7f,\"lon\":%.7f,\"alt\":%.2f,\"sats\":%lu,\"hdop\":%.2f,\"age_ms\":%lu,\"time\":\"%s\"}",
           gps.location.lat(),
           gps.location.lng(),
           gps.altitude.isValid() ? gps.altitude.meters() : 0.0,
           gps.satellites.isValid() ? gps.satellites.value() : 0,
           gps.hdop.isValid() ? gps.hdop.hdop() : 99.99,
           gps.location.age(),
           timeBuf);

  udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
  udp.write((const uint8_t*)packet, strlen(packet));
  udp.endPacket();

  Serial.print("UDP GPS: ");
  Serial.println(packet);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ORINBOT GPS monitor starting...");

  GPSserial.begin(115200, SERIAL_8N1, GPS_RX, -1);

  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(TFT_INVERT);
  tft.fillScreen(TFT_BLACK);

  connectWifi();

  udp.begin(UDP_TARGET_PORT);

  drawStaticUI();
  drawScreen();
}

void loop() {
  // Read GPS silently. No raw NMEA serial output.
  while (GPSserial.available()) {
    char c = GPSserial.read();
    gps.encode(c);
  }

  // Display update
  if (millis() - lastDraw >= 1000) {
    lastDraw = millis();
    drawScreen();
  }

  // UDP converted packet update
  if (millis() - lastUdp >= 1000) {
    lastUdp = millis();
    sendGpsUdp();
  }

  // WiFi reconnect check
  if (millis() - lastWifiCheck >= 5000) {
    lastWifiCheck = millis();

    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
      wifiConnected = false;
      Serial.println("WiFi lost.");
      drawStaticUI();
    }

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }
}
