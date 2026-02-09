#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Pin Definitions
#define Vext_Ctrl 3
#define LED_K 21
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42
#define GPS_RX 33
#define GPS_TX 34
#define SD_CS 4  // Pin CS untuk SD Card

#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""

// TODO: Ganti dengan URL Vercel Anda setelah deployment
const char* serverName = "https://your-vercel-app.vercel.app/api/track";

// Interval logging ke SD Card (dalam ms)
const unsigned long SD_LOG_INTERVAL = 5000; // 5 detik

TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

bool sdCardAvailable = false;
String currentLogFile = "";

// Fungsi untuk membuat nama file log berdasarkan tanggal
String generateLogFileName() {
  if (gps.date.isValid()) {
    char filename[20];
    sprintf(filename, "/GPS_%02d%02d%02d.csv", 
            gps.date.year() % 100, 
            gps.date.month(), 
            gps.date.day());
    return String(filename);
  }
  return "/GPS_LOG.csv";
}

// Fungsi untuk inisialisasi SD Card
bool initSDCard() {
  Serial.println("Initializing SD card...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  return true;
}

// Fungsi untuk menulis header CSV jika file baru
void writeCSVHeader(String filename) {
  if (!SD.exists(filename)) {
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      file.println("Timestamp,Date,Time,Latitude,Longitude,Altitude,Satellites,Speed,Course,HDOP");
      file.close();
      Serial.println("CSV header written");
    } else {
      Serial.println("Failed to create log file");
    }
  }
}

// Fungsi untuk logging data GPS ke SD Card
bool logGPSToSD(double lat, double lng) {
  if (!sdCardAvailable) return false;

  String logFile = generateLogFileName();
  
  // Update nama file jika berubah (ganti hari)
  if (currentLogFile != logFile) {
    currentLogFile = logFile;
    writeCSVHeader(currentLogFile);
  }

  File file = SD.open(currentLogFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open log file");
    return false;
  }

  // Format: Timestamp,Date,Time,Lat,Lng,Alt,Sats,Speed,Course,HDOP
  String logEntry = String(millis()) + ",";
  
  // Date
  if (gps.date.isValid()) {
    char dateStr[11];
    sprintf(dateStr, "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
    logEntry += String(dateStr);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // Time
  if (gps.time.isValid()) {
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    logEntry += String(timeStr);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // Location
  logEntry += String(lat, 6) + ",";
  logEntry += String(lng, 6) + ",";
  
  // Altitude
  if (gps.altitude.isValid()) {
    logEntry += String(gps.altitude.meters(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // Satellites
  if (gps.satellites.isValid()) {
    logEntry += String(gps.satellites.value());
  } else {
    logEntry += "0";
  }
  logEntry += ",";
  
  // Speed (km/h)
  if (gps.speed.isValid()) {
    logEntry += String(gps.speed.kmph(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // Course
  if (gps.course.isValid()) {
    logEntry += String(gps.course.deg(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // HDOP
  if (gps.hdop.isValid()) {
    logEntry += String(gps.hdop.hdop(), 2);
  } else {
    logEntry += "N/A";
  }

  file.println(logEntry);
  file.close();
  
  Serial.println("GPS logged: " + logEntry);
  return true;
}

void setup() {
  // 1. Power on Vext (GPS + TFT)
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  
  // 2. AKTIFKAN BACKLIGHT TFT
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  
  // 3. LED built-in
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(100);

  // 4. Serial untuk debugging & GPS
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // 5. Inisialisasi TFT
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  // 6. Clear display
  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  // 7. Inisialisasi SD Card
  sdCardAvailable = initSDCard();
  
  // 8. Tampilkan status SD Card di layar
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("SD Card: ");
  if (sdCardAvailable) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("FAIL");
  }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  // 9. Koneksi WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("Connecting WiFi...");
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    Serial.print(".");
    delay(500);
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed - continuing anyway");
  }

  Serial.println("GPS + TFT + SD + API Ready");
}

void loop() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  static unsigned long lastUpdate = 0;
  static unsigned long lastDebug = 0;
  static unsigned long lastSDLog = 0;
  
  // Debug GPS setiap 1 detik
  if (millis() - lastDebug >= 1000) {
    lastDebug = millis();
    Serial.print("Chars: ");
    Serial.print(gps.charsProcessed());
    Serial.print(" | Sentences: ");
    Serial.print(gps.sentencesWithFix());
    Serial.print(" | Failed: ");
    Serial.println(gps.failedChecksum());
  }
  
  // Update display setiap 5 detik
  if (millis() - lastUpdate >= 5000) {
    lastUpdate = millis();

    framebuffer.fillScreen(ST7735_BLACK);
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setTextSize(1);
    framebuffer.setCursor(5, 2);
    framebuffer.print("GPS+API+SD");
    
    // Status SD Card
    framebuffer.setCursor(100, 2);
    if (sdCardAvailable) {
      framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
      framebuffer.print("SD:OK");
    } else {
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.print("SD:X");
    }
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);

    if (gps.location.isValid()) {
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      int sats = gps.satellites.value();

      framebuffer.setCursor(5, 15);
      framebuffer.print("Lat:"); framebuffer.print(lat, 6);
      framebuffer.setCursor(5, 27);
      framebuffer.print("Lon:"); framebuffer.print(lng, 6);
      framebuffer.setCursor(5, 39);
      framebuffer.print("Sats:"); framebuffer.print(sats);

      // Tampilkan waktu jika tersedia
      if (gps.time.isValid()) {
        framebuffer.setCursor(70, 39);
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        framebuffer.print(timeStr);
      }

      Serial.print("GPS: ");
      Serial.print(lat, 6);
      Serial.print(", ");
      Serial.println(lng, 6);

      // WiFi & API
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000);

        String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(lat, 6) + "\",\"lng\":\"" + String(lng, 6) + "\"}";

        Serial.print("Sending: ");
        Serial.println(jsonData);
        
        int httpResponseCode = http.POST(jsonData);
        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);

        framebuffer.setCursor(5, 51);
        if(httpResponseCode > 0) {
           framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
           framebuffer.print("API: OK");
        } else {
           framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
           framebuffer.print("API: ERR");
        }
        http.end();
      } else {
        framebuffer.setCursor(5, 51);
        framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
        framebuffer.print("WiFi: Disc");
      }

      // Status SD Logging
      framebuffer.setCursor(5, 63);
      framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
      if (sdCardAvailable) {
        framebuffer.print("SD Logging...");
      } else {
        framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
        framebuffer.print("SD N/A");
      }

    } else {
      framebuffer.setCursor(25, 25);
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.setTextSize(2);
      framebuffer.print("NO FIX");
      framebuffer.setTextSize(1);
      Serial.println("Waiting for GPS fix...");
    }

    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }

  // Logging ke SD Card setiap interval tertentu
  if (gps.location.isValid() && sdCardAvailable) {
    if (millis() - lastSDLog >= SD_LOG_INTERVAL) {
      lastSDLog = millis();
      
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      
      bool logSuccess = logGPSToSD(lat, lng);
      
      // Blink LED jika berhasil log
      if (logSuccess) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
}
