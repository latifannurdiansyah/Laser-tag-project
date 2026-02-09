#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp> // Pastikan library IRremote terinstall

// Pin Definitions
#define Vext_Ctrl 3
#define LED_K 21
#define IR_RECEIVE_PIN 5

// TFT Display
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42

// GPS
#define GPS_RX 33
#define GPS_TX 34

// SD Card (SPI)
#define SD_CS   4
#define SD_SCK  9
#define SD_MOSI 10
#define SD_MISO 11

#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""

const char* serverName = "https://your-vercel-app.vercel.app/api/track";
const unsigned long SD_LOG_INTERVAL = 5000;
const unsigned long PAGE_SWITCH_INTERVAL = 5000; // Interval ganti halaman layar

// Struct untuk IR Payload
struct __attribute__((packed)) DataPayload {
    uint32_t shooter_address_id;
    uint8_t shooter_sub_address_id;
    uint8_t irHitLocation;
    uint8_t cheatDetected;
};

// Global Variables
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

bool sdCardAvailable = false;
String currentLogFile = "";
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;
bool irReceived = false;
int displayPage = 0; // 0 untuk GPS, 1 untuk IR

// --- Fungsi Pendukung SD Card ---
String generateLogFileName() {
  if (gps.date.isValid()) {
    char filename[20];
    sprintf(filename, "/GPS_%02d%02d%02d.csv", gps.date.year() % 100, gps.date.month(), gps.date.day());
    return String(filename);
  }
  return "/GPS_LOG.csv";
}

bool initSDCard() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) return false;
  if (SD.cardType() == CARD_NONE) return false;
  return true;
}

void writeCSVHeader(String filename) {
  if (!SD.exists(filename)) {
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      file.println("Timestamp,Date,Time,Latitude,Longitude,Altitude,Satellites,Speed,Course,HDOP");
      file.close();
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
  if (!file) return false;

  String logEntry = String(millis()) + ",";
  if (gps.date.isValid()) {
    char d[11]; sprintf(d, "%04d-%02d-%02d", gps.date.year(), gps.date.month(), gps.date.day());
    logEntry += String(d);
  } else logEntry += "N/A";
  logEntry += ",";
  
  if (gps.time.isValid()) {
    char t[9]; sprintf(t, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    logEntry += String(t);
  } else logEntry += "N/A";
  logEntry += ",";

  logEntry += String(lat, 6) + "," + String(lng, 6) + ",";
  logEntry += (gps.altitude.isValid() ? String(gps.altitude.meters()) : "N/A") + ",";
  logEntry += String(gps.satellites.value()) + ",";
  logEntry += (gps.speed.isValid() ? String(gps.speed.kmph()) : "N/A") + ",";
  logEntry += (gps.course.isValid() ? String(gps.course.deg()) : "N/A") + ",";
  logEntry += (gps.hdop.isValid() ? String(gps.hdop.hdop()) : "N/A");

  file.println(logEntry);
  file.close();
  return true;
}

void setup() {
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  
  sdCardAvailable = initSDCard();
  
  // Inisialisasi IR Receiver
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("System Ready");
}

void loop() {
  // 1. Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // 2. Baca data IR
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == NEC) {
      lastIRAddress = IrReceiver.decodedIRData.address;
      lastIRCommand = IrReceiver.decodedIRData.command;
      irReceived = true;
      Serial.printf("IR Received: Addr 0x%04X, Cmd 0x%02X\n", lastIRAddress, lastIRCommand);
    }
    IrReceiver.resume();
  }

  // 3. Logika Display dan API (Setiap 5 detik)
  static unsigned long lastUpdate = 0;
  static unsigned long lastSDLog = 0;

  if (millis() - lastUpdate >= PAGE_SWITCH_INTERVAL) {
    lastUpdate = millis();
    displayPage = (displayPage + 1) % 2; // Switch antara 0 dan 1

    framebuffer.fillScreen(ST7735_BLACK);
    framebuffer.setTextSize(1);
    
    if (displayPage == 0) {
      // --- HALAMAN 1: GPS & STATUS ---
      framebuffer.setCursor(5, 5);
      // Status SD
      framebuffer.setTextColor(sdCardAvailable ? ST7735_GREEN : ST7735_RED);
      framebuffer.print("SD:"); framebuffer.print(sdCardAvailable ? "OK" : "X");
      
      // Status WiFi
      framebuffer.setCursor(60, 5);
      bool wifiOk = (WiFi.status() == WL_CONNECTED);
      framebuffer.setTextColor(wifiOk ? ST7735_CYAN : ST7735_ORANGE);
      framebuffer.print("| Wifi:"); framebuffer.print(wifiOk ? "OK" : "X");

      if (gps.location.isValid()) {
        framebuffer.setTextColor(ST7735_WHITE);
        framebuffer.setCursor(5, 20);
        framebuffer.print("Lat: "); framebuffer.println(gps.location.lat(), 6);
        framebuffer.setCursor(5, 32);
        framebuffer.print("Lon: "); framebuffer.println(gps.location.lng(), 6);
        framebuffer.setCursor(5, 44);
        framebuffer.print("Alt: "); framebuffer.print(gps.altitude.meters(), 1); framebuffer.print("m");
        
        framebuffer.setCursor(5, 56);
        char timeBuf[20];
        sprintf(timeBuf, "Time: %02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        framebuffer.print(timeBuf);

        // Kirim API jika terkoneksi
        if (wifiOk) {
          HTTPClient http;
          http.begin(serverName);
          http.addHeader("Content-Type", "application/json");
          String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(gps.location.lat(), 6) + "\",\"lng\":\"" + String(gps.location.lng(), 6) + "\"}";
          int httpCode = http.POST(jsonData);
          http.end();
        }
      } else {
        framebuffer.setTextColor(ST7735_RED);
        framebuffer.setCursor(40, 40);
        framebuffer.print("WAITING GPS...");
      }
    } 
    else {
      // --- HALAMAN 2: DATA IR ---
      framebuffer.setTextColor(ST7735_YELLOW);
      framebuffer.setCursor(5, 5);
      framebuffer.print(">> IR RECEIVER DATA");
      framebuffer.drawFastHLine(0, 15, 160, ST7735_WHITE);

      framebuffer.setTextColor(ST7735_WHITE);
      framebuffer.setCursor(5, 25);
      framebuffer.print("Device ID: "); framebuffer.setTextColor(ST7735_CYAN); framebuffer.print("HELTEC-01");
      
      framebuffer.setTextColor(ST7735_WHITE);
      framebuffer.setCursor(5, 40);
      framebuffer.print("Address  : 0x"); framebuffer.print(lastIRAddress, HEX);
      
      framebuffer.setCursor(5, 55);
      framebuffer.print("Command  : 0x"); framebuffer.print(lastIRCommand, HEX);

      if (!irReceived) {
        framebuffer.setCursor(5, 70);
        framebuffer.setTextColor(ST7735_RED);
        framebuffer.print("No Signal Detected");
      } else {
        framebuffer.setCursor(5, 70);
        framebuffer.setTextColor(ST7735_GREEN);
        framebuffer.print("Signal Active");
      }
    }

    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }

  // 4. Logging SD Card (5 Detik sekali)
  if (gps.location.isValid() && sdCardAvailable && (millis() - lastSDLog >= SD_LOG_INTERVAL)) {
    lastSDLog = millis();
    if (logGPSToSD(gps.location.lat(), gps.location.lng())) {
      digitalWrite(LED_BUILTIN, HIGH); delay(50); digitalWrite(LED_BUILTIN, LOW);
    }
  }
}