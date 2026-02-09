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

// --- Konfigurasi WiFi & API ---
#define WIFI_SSID       "UserAndroid"
#define WIFI_PASSWORD   "55555550"
#define API_URL         "https://laser-tag-project.vercel.app/api/track"
#define DEVICE_ID       "Heltec-LaserTag-Dual"

// --- Konfigurasi LoRaWAN (TTN) ---
uint64_t devEUI = 0x70B3D57ED0075A41; 
uint64_t joinEUI = 0x0000000000000000;
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

// --- Objek Global ---
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &AS923); 
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

// Status Variables
bool sdCardAvailable = false;
bool wifiConnected = false;
bool isLoRaJoined = false;
bool irDataReceived = false;
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;
unsigned long lastIRTime = 0;
uint8_t currentPage = 0;
unsigned long lastPageSwitch = 0;

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
void logToSD(float lat, float lng) {
  if (!sdCardAvailable) return;
  File file = SD.open("/gps_log.csv", FILE_APPEND);
  if (file) {
    file.printf("%lu,%.6f,%.6f,%d\n", millis(), lat, lng, lastIRCommand);
    file.close();
    Serial.println("[SD] Logged");
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
    framebuffer.setTextColor(ST7735_GREEN);
    framebuffer.setCursor(2, 20);
    framebuffer.printf("LAT: %.6f", gps.location.lat());
    framebuffer.setCursor(2, 32);
    framebuffer.printf("LNG: %.6f", gps.location.lng());
    framebuffer.setCursor(2, 45);
    framebuffer.printf("SAT: %d", gps.satellites.value());
  } else { // Page IR
    framebuffer.setTextColor(ST7735_MAGENTA);
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
  delay(500);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  
  // Init TFT
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  
  // Init SD (Hardware SPI pins for Heltec)
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) sdCardAvailable = true;

  // Init IR
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  // Init WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Init LoRa & TTN
  int16_t state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) joinLoRaWAN();
}

void loop() {
  // 1. Baca GPS
  while (Serial1.available() > 0) gps.encode(Serial1.read());

  // 2. Baca IR
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == NEC) {
      lastIRCommand = IrReceiver.decodedIRData.command;
      irDataReceived = true;
      lastIRTime = millis();
    }
    IrReceiver.resume();
  }

  // 3. Update WiFi Status
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  // 4. Timer Tasks
  static unsigned long lastWiFiTask = 0;
  static unsigned long lastLoRaTask = 0;
  static unsigned long lastSDTask = 0;
  static unsigned long lastDispTask = 0;

  float currentLat = gps.location.lat();
  float currentLng = gps.location.lng();

  if (gps.location.isValid()) {
    // Kirim API WiFi
    if (millis() - lastWiFiTask >= API_SEND_INTERVAL) {
      sendToAPI(currentLat, currentLng);
      lastWiFiTask = millis();
    }
    // Kirim LoRa TTN
    if (millis() - lastLoRaTask >= LORA_INTERVAL) {
      sendLoRaData(currentLat, currentLng);
      lastLoRaTask = millis();
    }
    // Simpan ke SD
    if (millis() - lastSDTask >= SD_LOG_INTERVAL) {
      logToSD(currentLat, currentLng);
      lastSDTask = millis();
    }
  }

  // 5. Update Display & Switch Page
  if (millis() - lastPageSwitch >= PAGE_SWITCH_INTERVAL) {
    currentPage = !currentPage;
    lastPageSwitch = millis();
  }
  
  if (millis() - lastDispTask >= 1000) {
    updateDisplay();
    lastDispTask = millis();
  }
}