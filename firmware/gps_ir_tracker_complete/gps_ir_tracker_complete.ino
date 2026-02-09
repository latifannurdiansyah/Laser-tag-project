#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp>

// Pin Definitions
#define Vext_Ctrl 3
#define LED_K 21

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

// IR Receiver
#define IR_RECEIVE_PIN 5  // Pin untuk IR receiver (GANTI KE PIN 5)
#define ENABLE_LED_FEEDBACK false

#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""

// Static IP Configuration
#define USE_STATIC_IP true  // Set false jika mau pakai DHCP
IPAddress local_IP(192, 168, 11, 242);      // IP yang diinginkan
IPAddress gateway(192, 168, 11, 1);         // Gateway router (biasanya 192.168.11.1)
IPAddress subnet(255, 255, 255, 0);         // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);           // DNS Google (optional)
IPAddress secondaryDNS(8, 8, 4, 4);         // DNS Google secondary (optional)

// Ganti dengan URL Vercel Anda
const char* serverName = "https://your-vercel-app.vercel.app/api/track";

// GPS Baud Rate
#define GPS_BAUD_RATE 115200

// Interval
const unsigned long SD_LOG_INTERVAL = 5000;      // 5 detik
const unsigned long SCREEN_SCROLL_INTERVAL = 3000; // 3 detik ganti halaman

TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

bool sdCardAvailable = false;
String currentLogFile = "";
unsigned long gpsStartTime = 0;
int rawGPSChars = 0;
String wifiIP = "N/A";

// Screen scrolling
uint8_t currentPage = 0;  // 0 = GPS Page, 1 = IR Page
unsigned long lastPageSwitch = 0;

// IR Data Storage
struct __attribute__((packed)) DataPayload {
    uint32_t shooter_address_id;
    uint8_t shooter_sub_address_id;
    uint8_t irHitLocation;
    uint8_t cheatDetected;
};

DataPayload payload;

// IR Last Received Data
bool irDataReceived = false;
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;
String lastIRProtocol = "NONE";
unsigned long lastIRTime = 0;

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
  
  // Inisialisasi SPI dengan pin custom untuk SD Card
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
  
  Serial.println("GPS logged: " + logEntry);
  return true;
}

// Fungsi untuk menampilkan PAGE 1: GPS Data
void displayGPSPage() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextSize(1);
  
  // === HEADER LINE ===
  framebuffer.setCursor(2, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("GPS+API+SD");
  
  // SD Status
  framebuffer.setCursor(75, 2);
  framebuffer.print("|SD:");
  if (sdCardAvailable) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  // WiFi Status
  framebuffer.setCursor(110, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("|W:");
  if (WiFi.status() == WL_CONNECTED) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  // === GPS DATA ===
  if (gps.location.isValid()) {
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    
    // Latitude
    framebuffer.setCursor(2, 16);
    framebuffer.print("Lat:");
    framebuffer.print(lat, 6);
    
    // Longitude
    framebuffer.setCursor(2, 28);
    framebuffer.print("Lng:");
    framebuffer.print(lng, 6);
    
    // Altitude
    framebuffer.setCursor(2, 40);
    framebuffer.print("Alt:");
    if (gps.altitude.isValid()) {
      framebuffer.print(gps.altitude.meters(), 1);
      framebuffer.print("m");
    } else {
      framebuffer.print("N/A");
    }
    
    // Satellites
    framebuffer.setCursor(80, 40);
    framebuffer.print("Sat:");
    framebuffer.print(gps.satellites.value());
    
    // Timestamp
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
    
    // WiFi IP
    framebuffer.setCursor(2, 64);
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print("IP:");
    framebuffer.print(wifiIP);
    
  } else {
    // NO GPS FIX
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
  
  // Page indicator
  framebuffer.setCursor(140, 72);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("P1");
  
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

// Fungsi untuk menampilkan PAGE 2: IR Receiver Data
void displayIRPage() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextSize(1);
  
  // === HEADER LINE (sama seperti Page 1) ===
  framebuffer.setCursor(2, 2);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.print("GPS+API+SD");
  
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
  if (WiFi.status() == WL_CONNECTED) {
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print("OK");
  } else {
    framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
    framebuffer.print("X");
  }
  
  // === IR RECEIVER TITLE ===
  framebuffer.setCursor(2, 16);
  framebuffer.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("IR RECEIVER");
  
  // === IR DATA ===
  if (irDataReceived) {
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    
    // Protocol
    framebuffer.setCursor(2, 30);
    framebuffer.print("Protocol:");
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print(lastIRProtocol);
    
    // Address
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setCursor(2, 42);
    framebuffer.print("Address: 0x");
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print(lastIRAddress, HEX);
    
    // Command
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setCursor(2, 54);
    framebuffer.print("Command: 0x");
    framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
    framebuffer.print(lastIRCommand, HEX);
    
    // Time since last IR
    framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
    framebuffer.setCursor(2, 66);
    unsigned long timeSince = (millis() - lastIRTime) / 1000;
    framebuffer.printf("Last: %lus ago", timeSince);
    
  } else {
    // Waiting for IR signal
    framebuffer.setCursor(10, 35);
    framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    framebuffer.print("Waiting for");
    framebuffer.setCursor(10, 50);
    framebuffer.print("IR signal...");
  }
  
  // Page indicator
  framebuffer.setCursor(140, 72);
  framebuffer.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
  framebuffer.print("P2");
  
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void setup() {
  // 1. Power on Vext (GPS + TFT)
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, LOW);
  delay(100);
  digitalWrite(Vext_Ctrl, HIGH);
  
  // 2. AKTIFKAN BACKLIGHT TFT
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  
  // 3. LED built-in
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(500);

  // 4. Serial untuk debugging & GPS
  Serial.begin(115200);
  
  Serial.println("\n========================================");
  Serial.println("GPS Tracker + IR Receiver + SD Card");
  Serial.printf("GPS Baud: %d\n", GPS_BAUD_RATE);
  Serial.println("========================================\n");
  
  Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX, GPS_TX);
  delay(1000);
  gpsStartTime = millis();

  // 5. Inisialisasi IR Receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR Receiver Ready on Pin 5");

  // 6. Inisialisasi TFT
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_CYAN, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("GPS Tracker");
  framebuffer.setCursor(5, 20);
  framebuffer.print("+ IR Receiver");
  framebuffer.setCursor(5, 35);
  framebuffer.printf("Baud: %d", GPS_BAUD_RATE);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  delay(1500);

  // 7. Inisialisasi SD Card
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

  // 8. Koneksi WiFi dengan Static IP
  Serial.println("Configuring WiFi...");
  
  // Konfigurasi Static IP (jika USE_STATIC_IP = true)
  if (USE_STATIC_IP) {
    Serial.println("Setting Static IP: 192.168.11.242");
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("Static IP configuration failed!");
    }
  }
  
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  framebuffer.print("Connecting WiFi...");
  framebuffer.setCursor(5, 20);
  framebuffer.print("IP: 192.168.11.242");
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    Serial.print(".");
    delay(500);
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    wifiIP = WiFi.localIP().toString();
    Serial.print("IP Address: ");
    Serial.println(wifiIP);
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
  } else {
    Serial.println("\nWiFi Failed");
    wifiIP = "N/A";
  }

  Serial.println("\nGPS + TFT + SD + API + IR Ready\n");
  
  lastPageSwitch = millis();
}

void loop() {
  // === BACA GPS ===
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    rawGPSChars++;
    gps.encode(c);
  }

  // === BACA IR RECEIVER ===
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
      IrReceiver.printIRSendUsage(&Serial);
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
  
  // Debug GPS setiap 2 detik
  if (millis() - lastDebug >= 2000) {
    lastDebug = millis();
    Serial.printf("GPS: Chars=%d | Proc=%d | Valid=%d | Sats=%d\n",
                  rawGPSChars,
                  gps.charsProcessed(),
                  gps.sentencesWithFix(),
                  gps.satellites.value());
  }
  
  // === AUTO SCROLL DISPLAY (ganti halaman setiap 3 detik) ===
  if (millis() - lastPageSwitch >= SCREEN_SCROLL_INTERVAL) {
    lastPageSwitch = millis();
    currentPage = (currentPage + 1) % 2;  // Toggle 0 <-> 1
  }
  
  // === UPDATE DISPLAY ===
  if (millis() - lastUpdate >= 1000) {  // Update setiap 1 detik
    lastUpdate = millis();
    
    if (currentPage == 0) {
      displayGPSPage();
    } else {
      displayIRPage();
    }
  }

  // === LOGGING KE SD CARD ===
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

  // === KIRIM KE API (hanya jika GPS valid) ===
  static unsigned long lastAPIUpload = 0;
  if (gps.location.isValid() && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastAPIUpload >= 10000) {  // Setiap 10 detik
      lastAPIUpload = millis();
      
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      http.setTimeout(5000);

      double lat = gps.location.lat();
      double lng = gps.location.lng();
      String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(lat, 6) + "\",\"lng\":\"" + String(lng, 6) + "\"}";

      Serial.print("Sending to API: ");
      Serial.println(jsonData);
      
      int httpResponseCode = http.POST(jsonData);
      Serial.printf("HTTP Response: %d\n", httpResponseCode);
      http.end();
    }
  }
}
