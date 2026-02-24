#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp>

#define WIFI_SSID       "UserAndroid"
#define WIFI_PASSWORD   "55555550"

// Dual Mode: true = Kirim ke HTTP lokal DAN HTTPS Vercel
#define USE_DUAL_MODE   true

// HTTPS Vercel
#define API_URL         "https://laser-tag-project.vercel.app/api/track"

// HTTP Lokal (MySQL)
#define LOCAL_API_URL   "http://192.168.122.226/lasertag/api/track.php"

#define DEVICE_ID       "Heltec-IR-V1"

#define Vext_Ctrl 3
#define LED_K 21
#define IR_RECEIVE_PIN 5

#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42

#define GPS_RX 33
#define GPS_TX 34

#define SD_CS   4
#define SD_SCK  9
#define SD_MOSI 10
#define SD_MISO 11

#define WIFI_TIMEOUT 15000
#define WIFI_RETRY_INTERVAL 5000
#define API_SEND_INTERVAL 5000
#define SD_LOG_INTERVAL 5000
#define PAGE_SWITCH_INTERVAL 3000

TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

bool sdCardAvailable = false;
String currentLogFile = "";
unsigned long gpsStartTime = 0;
String wifiIP = "N/A";
bool wifiConnected = false;
unsigned long lastWiFiReconnectAttempt = 0;

uint8_t currentPage = 0;
unsigned long lastPageSwitch = 0;

bool irDataReceived = false;
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;
String lastIRProtocol = "NONE";
unsigned long lastIRTime = 0;

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

bool initSDCard() {
  Serial.println("Initializing SD card...");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
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

bool logGPSToSD(double lat, double lng) {
  if (!sdCardAvailable) return false;

  String logFile = generateLogFileName();
  
  if (currentLogFile != logFile) {
    currentLogFile = logFile;
    writeCSVHeader(currentLogFile);
  }

  File file = SD.open(currentLogFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open log file");
    return false;
  }

  String logEntry = String(millis()) + ",";
  
  if (gps.date.isValid()) {
    char dateStr[11];
    sprintf(dateStr, "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
    logEntry += String(dateStr);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  if (gps.time.isValid()) {
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    logEntry += String(timeStr);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  logEntry += String(lat, 6) + ",";
  logEntry += String(lng, 6) + ",";
  
  if (gps.altitude.isValid()) {
    logEntry += String(gps.altitude.meters(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  if (gps.satellites.isValid()) {
    logEntry += String(gps.satellites.value());
  } else {
    logEntry += "0";
  }
  logEntry += ",";
  
  if (gps.speed.isValid()) {
    logEntry += String(gps.speed.kmph(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  if (gps.course.isValid()) {
    logEntry += String(gps.course.deg(), 2);
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  if (gps.hdop.isValid()) {
    logEntry += String(gps.hdop.hdop(), 2);
  } else {
    logEntry += "N/A";
  }

  file.println(logEntry);
  file.close();
  
  Serial.println("GPS logged to SD: " + logEntry);
  return true;
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    wifiIP = WiFi.localIP().toString();
    Serial.print("IP Address: ");
    Serial.println(wifiIP);
    wifiConnected = true;
  } else {
    Serial.println("\nWiFi connection failed - continuing anyway");
    wifiIP = "N/A";
    wifiConnected = false;
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    if (millis() - lastWiFiReconnectAttempt >= WIFI_RETRY_INTERVAL) {
      lastWiFiReconnectAttempt = millis();
      Serial.println("WiFi disconnected, reconnecting...");
      WiFi.reconnect();
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("WiFi reconnected! IP: " + WiFi.localIP().toString());
  }
}

void sendToAPI(float lat, float lng) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] WiFi not connected, skipping...");
    return;
  }

  // Format IR Status: "HIT: 0xXXXX-0xXX" atau "-"
  String irStatus = irDataReceived ? 
    String("HIT: 0x") + String(lastIRAddress, HEX) + String("-0x") + String(lastIRCommand, HEX) : 
    "-";

  String payload = String("{") +
    "\"id\":\"" DEVICE_ID "\"," +
    "\"lat\":" + String(lat, 6) + "," +
    "\"lng\":" + String(lng, 6) + "," +
    "\"irStatus\":\"" + irStatus + "\"" +
    "}";

  Serial.print("[API] Payload: ");
  Serial.println(payload);

#if USE_DUAL_MODE
  // Kirim ke DUA endpoint sekaligus: HTTP Lokal + HTTPS Vercel
  bool okLocal = false, okVercel = false;
  int codeLocal = 0, codeVercel = 0;

  // 1. Kirim ke HTTP Lokal
  {
    HTTPClient http;
    http.begin(LOCAL_API_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    codeLocal = http.POST(payload);
    okLocal = (codeLocal > 0);
    Serial.printf("[API] Local: %s (code: %d)\n", okLocal?"OK":"FAIL", codeLocal);
    http.end();
  }

  // 2. Kirim ke HTTPS Vercel
  {
    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    codeVercel = http.POST(payload);
    okVercel = (codeVercel > 0);
    Serial.printf("[API] Vercel: %s (code: %d)\n", okVercel?"OK":"FAIL", codeVercel);
    http.end();
  }

  if (!okLocal && !okVercel) {
    Serial.println("[API] Both failed!");
  }
#else
  // Mode tunggal - hanya Vercel
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("[API] Success! Response: %d\n", httpResponseCode);
  } else {
    Serial.printf("[API] Error: %d\n", httpResponseCode);
  }
  
  http.end();
#endif
}

void displayGPSPage() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextSize(1);
  
  framebuffer.setCursor(2, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("GPS+SD+API");
  
  framebuffer.setCursor(75, 2);
  framebuffer.print("|SD:");
  if (sdCardAvailable) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  framebuffer.setCursor(110, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("|W:");
  if (wifiConnected) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  if (gps.location.isValid()) {
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    
    framebuffer.setCursor(2, 16);
    framebuffer.print("Lat:");
    framebuffer.print(lat, 6);
    
    framebuffer.setCursor(2, 28);
    framebuffer.print("Lng:");
    framebuffer.print(lng, 6);
    
    framebuffer.setCursor(2, 40);
    framebuffer.print("Alt:");
    if (gps.altitude.isValid()) {
      framebuffer.print(gps.altitude.meters(), 1);
      framebuffer.print("m");
    } else {
      framebuffer.print("N/A");
    }
    
    framebuffer.setCursor(80, 40);
    framebuffer.print("Sat:");
    framebuffer.print(gps.satellites.value());
    
    framebuffer.setCursor(2, 52);
    framebuffer.print("Time:");
    if (gps.time.isValid()) {
      char timeStr[9];
      sprintf(timeStr, "%02d:%02d:%02d", 
              gps.time.hour(), 
              gps.time.minute(), 
              gps.time.second());
      framebuffer.print(timeStr);
    } else {
      framebuffer.print("N/A");
    }
    
    framebuffer.setCursor(2, 64);
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print("IP:");
    framebuffer.print(wifiIP);
    
  } else {
    framebuffer.setCursor(30, 30);
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.setTextSize(2);
    framebuffer.print("NO FIX");
    
    framebuffer.setTextSize(1);
    framebuffer.setCursor(2, 55);
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    unsigned long elapsed = (millis() - gpsStartTime) / 1000;
    framebuffer.printf("Wait:%lus Sat:%d", elapsed, gps.satellites.value());
  }
  
  framebuffer.setCursor(140, 72);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("P1");
  
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void displayIRPage() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextSize(1);
  
  framebuffer.setCursor(2, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("GPS+SD+API");
  
  framebuffer.setCursor(75, 2);
  framebuffer.print("|SD:");
  if (sdCardAvailable) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  framebuffer.setCursor(110, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("|W:");
  if (wifiConnected) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  framebuffer.setCursor(2, 16);
  framebuffer.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  framebuffer.print("IR RECEIVER");
  
  if (irDataReceived) {
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    
    framebuffer.setCursor(2, 30);
    framebuffer.print("Protocol:");
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print(lastIRProtocol);
    
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setCursor(2, 42);
    framebuffer.print("Address: 0x");
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print(lastIRAddress, HEX);
    
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setCursor(2, 54);
    framebuffer.print("Command: 0x");
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print(lastIRCommand, HEX);
    
    framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
    framebuffer.setCursor(2, 66);
    unsigned long timeSince = (millis() - lastIRTime) / 1000;
    framebuffer.printf("Last: %lus ago", timeSince);
    
  } else {
    framebuffer.setCursor(10, 35);
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print("Waiting for");
    framebuffer.setCursor(10, 50);
    framebuffer.print("IR signal...");
  }
  
  framebuffer.setCursor(140, 72);
  framebuffer.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  framebuffer.print("P2");
  
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void setup() {
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, LOW);
  delay(100);
  digitalWrite(Vext_Ctrl, HIGH);
  
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(500);

  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("GPS Tracker + IR Receiver + SD Card + Vercel API");
  Serial.println("Device ID: " DEVICE_ID);
  Serial.println("API URL: " API_URL);
  Serial.println("========================================\n");
  
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  delay(1000);
  gpsStartTime = millis();

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.println("IR Receiver Ready on Pin 5");

  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("GPS+IR+SD+API");
  framebuffer.setCursor(5, 20);
  framebuffer.print(DEVICE_ID);
  framebuffer.setCursor(5, 35);
  framebuffer.print("Initializing...");
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  sdCardAvailable = initSDCard();
  
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
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

  connectWiFi();

  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  if (wifiConnected) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("WiFi: ");
    framebuffer.print(wifiIP);
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("WiFi: FAILED");
  }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  Serial.println("\nGPS + TFT + SD + IR + API Ready\n");
  
  lastPageSwitch = millis();
}

void loop() {
  checkWiFiConnection();

  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  if (IrReceiver.decode()) {
    Serial.println("\n=== IR Signal Received! ===");
    IrReceiver.printIRResultShort(&Serial);
    
    if (IrReceiver.decodedIRData.protocol == NEC) {
      lastIRProtocol = "NEC";
      lastIRAddress = IrReceiver.decodedIRData.address;
      lastIRCommand = IrReceiver.decodedIRData.command;
      irDataReceived = true;
      lastIRTime = millis();
      
      Serial.printf("NEC | Addr: 0x%04X | Cmd: 0x%02X\n", lastIRAddress, lastIRCommand);
    } else {
      lastIRProtocol = "UNKNOWN";
      Serial.printf("Unknown Protocol: %d\n", IrReceiver.decodedIRData.protocol);
    }
    
    Serial.println("===========================\n");
    IrReceiver.resume();
  }

  static unsigned long lastUpdate = 0;
  static unsigned long lastDebug = 0;
  static unsigned long lastSDLog = 0;
  static unsigned long lastAPIUpload = 0;
  
  if (millis() - lastDebug >= 2000) {
    lastDebug = millis();
    Serial.printf("GPS: Chars=%d | Valid=%d | Sats=%d\n",
                  gps.charsProcessed(),
                  gps.location.isValid(),
                  gps.satellites.value());
  }
  
  if (millis() - lastPageSwitch >= PAGE_SWITCH_INTERVAL) {
    lastPageSwitch = millis();
    currentPage = (currentPage + 1) % 2;
  }
  
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    
    if (currentPage == 0) {
      displayGPSPage();
    } else {
      displayIRPage();
    }
  }

  if (gps.location.isValid() && sdCardAvailable) {
    if (millis() - lastSDLog >= SD_LOG_INTERVAL) {
      lastSDLog = millis();
      
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      
      bool logSuccess = logGPSToSD(lat, lng);
      
      if (logSuccess) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }

  if (gps.location.isValid() && wifiConnected) {
    if (millis() - lastAPIUpload >= API_SEND_INTERVAL) {
      lastAPIUpload = millis();
      
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      
      sendToAPI(lat, lng);
    }
  }
}
