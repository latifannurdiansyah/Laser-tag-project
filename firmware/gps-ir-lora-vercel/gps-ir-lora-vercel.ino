#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp>
#include <RadioLib.h>

// ============================================
// KONFIGURASI - UBAH DI SINI SEBELUM UPLOAD
// ============================================
#define WIFI_SSID       "UserAndroid"        // Ubah SSID WiFi
#define WIFI_PASSWORD   "55555550"          // Ubah Password WiFi
#define API_URL         "https://laser-tag-project.vercel.app/api/track"
#define DEVICE_ID       "Player 1"

// LoRaWAN TTN Configuration
#define LORA_DEV_EUI    "70B3D57ED0075A41"
#define LORA_APP_KEY    "D0DFF00C243B3BAA1FE261FBAD3DD72C"
#define LORA_NWK_KEY    "D0DFF00C243B3BAA1FE261FBAD3DD72C"
// ============================================

// --- Konfigurasi LoRaWAN (TTN) ---
uint64_t devEUI =   0x70B3D57ED0075A41; 
uint64_t joinEUI =  0x0000000000000000;
uint8_t appKey[] = { 0xD0, 0xDF, 0xF0, 0x0C, 0x24, 0xCB, 0x3B, 0xAA, 0x1F, 0xE2, 0x61, 0xFB, 0xAD, 0x3D, 0xD7, 0x2C };
uint8_t nwkKey[] = { 0xD0, 0xDF, 0xF0, 0x0C, 0x24, 0xCB, 0x3B, 0xAA, 0x1F, 0xE2, 0x61, 0xFB, 0xAD, 0x3D, 0xD7, 0x2C };

// --- Pinout Heltec Tracker V1.1 ---
#define RADIO_CS_PIN    8
#define RADIO_DIO1_PIN  14
#define RADIO_RST_PIN   12
#define RADIO_BUSY_PIN  13

#define Vext_Ctrl       3
#define LED_K           21
#define IR_RECEIVE_PIN  5

#define TFT_CS          38
#define TFT_DC          40
#define TFT_RST         39
#define TFT_SCLK        41
#define TFT_MOSI        42

#define GPS_RX          33
#define GPS_TX          34

#define SD_CS           4
#define SD_SCK          9
#define SD_MOSI         10
#define SD_MISO         11

// --- Interval Timers ---
#define API_SEND_INTERVAL   10000  // 10 detik ke WiFi
#define LORA_INTERVAL       30000  // 30 detik ke LoRaWAN (Fair Use)
#define SD_LOG_INTERVAL     5000   // 5 detik ke SD Card
#define PAGE_SWITCH_INTERVAL 3000
#define WIFI_TIMEOUT        15000  // 15 detik timeout WiFi
#define WIFI_RETRY_INTERVAL 5000   // 5 detik retry WiFi

// --- Objek Global ---
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &AS923); 
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

bool sdCardAvailable = false;
String currentLogFile = "";
unsigned long gpsStartTime = 0;
String wifiIP = "N/A";
bool wifiConnected = false;
unsigned long lastWiFiReconnectAttempt = 0;
bool isLoRaJoined = false;

uint8_t currentPage = 0;
unsigned long lastPageSwitch = 0;

bool irDataReceived = false;
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;
String lastIRProtocol = "NONE";
unsigned long lastIRTime = 0;

// --- Fungsi LoRaWAN ---
void joinLoRaWAN() {
  Serial.println(F("[LoRa] Mencoba Join TTN..."));
  int16_t state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state == RADIOLIB_ERR_NONE) {
    isLoRaJoined = true;
    Serial.println(F("[LoRa] Join Berhasil!"));
  } else {
    Serial.printf("[LoRa] Join Gagal: %d\n", state);
  }
}

void sendLoRaData(float lat, float lng) {
  if (!isLoRaJoined) return;
  
  int32_t latInt = lat * 1000000;
  int32_t lngInt = lng * 1000000;
  
  uint8_t payload[9];
  payload[0] = (latInt >> 24) & 0xFF; payload[1] = (latInt >> 16) & 0xFF;
  payload[2] = (latInt >> 8) & 0xFF;  payload[3] = latInt & 0xFF;
  payload[4] = (lngInt >> 24) & 0xFF; payload[5] = (lngInt >> 16) & 0xFF;
  payload[6] = (lngInt >> 8) & 0xFF;  payload[7] = lngInt & 0xFF;
  payload[8] = irDataReceived ? lastIRCommand : 0;

  int16_t state = node.sendReceive(payload, 9);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[LoRa] Uplink Berhasil"));
    irDataReceived = false; 
  }
}

// --- Fungsi WiFi API ---
void sendToAPI(float lat, float lng) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  String irStatus = irDataReceived ? "HIT: 0x" + String(lastIRCommand, HEX) : "-";
  String payload = "{\"id\":\"" + String(DEVICE_ID) + "\",\"lat\":" + String(lat, 6) + 
                   ",\"lng\":" + String(lng, 6) + ",\"irStatus\":\"" + irStatus + "\"}";

  int httpCode = http.POST(payload);
  Serial.printf("[API] Code: %d\n", httpCode);
  http.end();
}

// --- Fungsi SD Card ---
bool initSDCard() {
  Serial.println(F("Initializing SD card..."));
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD Card initialization failed!"));
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println(F("No SD card attached"));
    return false;
  }

  Serial.print(F("SD Card Type: "));
  if (cardType == CARD_MMC) {
    Serial.println(F("MMC"));
  } else if (cardType == CARD_SD) {
    Serial.println(F("SDSC"));
  } else if (cardType == CARD_SDHC) {
    Serial.println(F("SDHC"));
  } else {
    Serial.println(F("UNKNOWN"));
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  return true;
}

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

void writeCSVHeader(String filename) {
  if (!SD.exists(filename)) {
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      file.println("Timestamp,Date,Time,Latitude,Longitude,Altitude,Satellites,Speed,Course,HDOP");
      file.close();
      Serial.println(F("CSV header written"));
    } else {
      Serial.println(F("Failed to create log file"));
    }
  }
}

void logToSD(float lat, float lng) {
  if (!sdCardAvailable) return;

  String logFile = generateLogFileName();
  
  if (currentLogFile != logFile) {
    currentLogFile = logFile;
    writeCSVHeader(currentLogFile);
  }

  File file = SD.open(currentLogFile, FILE_APPEND);
  if (!file) {
    Serial.println(F("Failed to open log file"));
    return;
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
  
  Serial.println(F("GPS logged to SD"));
}

// --- Fungsi WiFi ---
void connectWiFi() {
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWiFi Connected!"));
    wifiIP = WiFi.localIP().toString();
    Serial.print(F("IP Address: "));
    Serial.println(wifiIP);
    wifiConnected = true;
  } else {
    Serial.println(F("\nWiFi connection failed"));
    wifiIP = "N/A";
    wifiConnected = false;
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    if (millis() - lastWiFiReconnectAttempt >= WIFI_RETRY_INTERVAL) {
      lastWiFiReconnectAttempt = millis();
      Serial.println(F("WiFi disconnected, reconnecting..."));
      WiFi.reconnect();
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println(F("WiFi reconnected!"));
  }
}

// --- Tampilan Layar ---
void updateDisplay() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(2, 2);
  
  // Status Bar
  framebuffer.printf("SD:%s W:%s L:%s", 
    sdCardAvailable ? "OK" : "X", 
    wifiConnected ? "OK" : "X", 
    isLoRaJoined ? "OK" : "X");

   if (currentPage == 0) { // Page GPS
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.setCursor(2, 20);
     framebuffer.printf("LAT: %.6f", gps.location.lat());
     framebuffer.setCursor(2, 32);
     framebuffer.printf("LNG: %.6f", gps.location.lng());
     framebuffer.setCursor(2, 45);
     framebuffer.printf("SAT: %d", gps.satellites.value());
   } else { // Page IR
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.setCursor(2, 20);
     framebuffer.print("IR RECEIVER");
     framebuffer.setCursor(2, 35);
     if (irDataReceived) {
       framebuffer.printf("CMD: 0x%02X", lastIRCommand);
       framebuffer.setCursor(2, 47);
       framebuffer.printf("Last: %lus ago", (millis() - lastIRTime)/1000);
     } else {
       framebuffer.print("Waiting...");
     }
   }
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
  Serial.println(F("\n========================================"));
  Serial.println(F("GPS Tracker + IR + LoRaWAN + SD + Vercel API"));
  Serial.print(F("Device ID: "));
  Serial.println(DEVICE_ID);
  Serial.print(F("API URL: "));
  Serial.println(API_URL);
  Serial.println(F("========================================\n"));
  
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  delay(1000);
  gpsStartTime = millis();

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.println(F("IR Receiver Ready on Pin 5"));

  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("GPS+LoRa+IR+SD");
  framebuffer.setCursor(5, 20);
  framebuffer.print(DEVICE_ID);
  framebuffer.setCursor(5, 35);
  framebuffer.print("Initializing...");
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  sdCardAvailable = initSDCard();
  
   framebuffer.fillScreen(ST7735_BLACK);
   framebuffer.setCursor(5, 5);
   framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
   framebuffer.print("SD Card: ");
   if (sdCardAvailable) {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("OK");
   } else {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("FAIL");
   }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  connectWiFi();

   framebuffer.fillScreen(ST7735_BLACK);
   framebuffer.setCursor(5, 5);
   if (wifiConnected) {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("WiFi: ");
     framebuffer.print(wifiIP);
   } else {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("WiFi: FAILED");
   }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  if (!isLoRaJoined) {
    joinLoRaWAN();
  }
  
   framebuffer.fillScreen(ST7735_BLACK);
   framebuffer.setCursor(5, 5);
   if (isLoRaJoined) {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("LoRaWAN: JOINED");
   } else {
     framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
     framebuffer.print("LoRaWAN: FAILED");
   }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  Serial.println(F("\nGPS + TFT + SD + IR + LoRaWAN + API Ready\n"));
  
  lastPageSwitch = millis();
}

void loop() {
  checkWiFiConnection();

  while (Serial1.available() > 0) gps.encode(Serial1.read());

  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == NEC) {
      lastIRProtocol = "NEC";
      lastIRAddress = IrReceiver.decodedIRData.address;
      lastIRCommand = IrReceiver.decodedIRData.command;
      irDataReceived = true;
      lastIRTime = millis();
      
      Serial.printf("IR Received | Addr: 0x%04X | Cmd: 0x%02X\n", lastIRAddress, lastIRCommand);
    }
    IrReceiver.resume();
  }

  static unsigned long lastUpdate = 0;
  static unsigned long lastDebug = 0;
  static unsigned long lastSDLog = 0;
  static unsigned long lastAPIUpload = 0;
  static unsigned long lastLoRaTask = 0;
  
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
    updateDisplay();
  }

  if (gps.location.isValid() && sdCardAvailable) {
    if (millis() - lastSDLog >= SD_LOG_INTERVAL) {
      lastSDLog = millis();
      logToSD(gps.location.lat(), gps.location.lng());
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }

  if (gps.location.isValid() && wifiConnected) {
    if (millis() - lastAPIUpload >= API_SEND_INTERVAL) {
      lastAPIUpload = millis();
      sendToAPI(gps.location.lat(), gps.location.lng());
    }
  }

  if (gps.location.isValid() && isLoRaJoined) {
    if (millis() - lastLoRaTask >= LORA_INTERVAL) {
      lastLoRaTask = millis();
      sendLoRaData(gps.location.lat(), gps.location.lng());
    }
  }
}