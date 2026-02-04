/*
  GPS Tracker - FIREBASE VERSION (SIM900A)
  Menggunakan REST API Firebase melalui AT Commands
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// ===== PIN DEFINITIONS =====
#define Vext_Ctrl  3
#define LED_K      21
#define TFT_CS     38
#define TFT_DC     40
#define TFT_RST    39
#define TFT_SCLK   41
#define TFT_MOSI   42
#define GPS_RX     33
#define GPS_TX     34
#define GPRS_TX_PIN 17
#define GPRS_RX_PIN 16
#define GPRS_RST_PIN 15

// ===== CONFIGURATION =====
const char* APN = "indosatgprs";
// Pastikan HOST tidak pakai https:// dan PATH diakhiri dengan .json
const char* FIREBASE_HOST = "gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app";
const char* FIREBASE_PATH = "/locations.json"; 
const char* FIREBASE_AUTH = "AMTJsG2px3hwlumdLkvcpnFNQF3hybkw0WvwM9WI"; 
const char* DEVICE_ID = "heltec-tracker-01";

// ===== GLOBAL OBJECTS =====
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);
HardwareSerial sim900(2);

bool gprsReady = false;

// ===== FORWARD DECLARATIONS =====
void flushSIM900A(unsigned long timeout);
String readResponse(unsigned long timeout);
bool sendAT(const char* cmd, unsigned long timeout);
bool activateGPRS();
bool sendToFirebase(double lat, double lng);
void updateDisplay();
void showStatus(const char* msg);

// ===== UTILITY FUNCTIONS =====
void flushSIM900A(unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (sim900.available()) sim900.read();
  }
}

String readResponse(unsigned long timeout) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (sim900.available()) {
      char c = sim900.read();
      response += c;
      Serial.write(c);
    }
    if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0 ||
        response.indexOf("+HTTPACTION:") >= 0 || response.indexOf("DOWNLOAD") >= 0) {
      break;
    }
    delay(10);
  }
  return response;
}

bool sendAT(const char* cmd, unsigned long timeout) {
  Serial.print("→ ");
  Serial.println(cmd);
  flushSIM900A(100);
  sim900.println(cmd);
  String response = readResponse(timeout);
  return (response.indexOf("OK") >= 0);
}

// ===== GPRS ACTIVATION =====
bool activateGPRS() {
  Serial.println("\n[GPRS] Activating...");
  sendAT("AT+CIPSHUT", 5000);
  delay(1000);
  
  if (!sendAT("AT+CGATT=1", 15000)) return false;
  
  String apnCmd = "AT+CSTT=\"" + String(APN) + "\",\"\",\"\"";
  if (!sendAT(apnCmd.c_str(), 8000)) return false;
  
  if (!sendAT("AT+CIICR", 25000)) return false;
  
  sim900.println("AT+CIFSR");
  String response = readResponse(5000);
  return (response.indexOf(".") >= 0);
}

// ===== FIREBASE SEND (REST API) =====
bool sendToFirebase(double lat, double lng) {
  Serial.println("\n[FIREBASE] Starting POST...");
  flushSIM900A(500);
  
  if (!sendAT("AT+HTTPINIT", 5000)) return false;
  sendAT("AT+HTTPPARA=\"CID\",1", 2000);
  
  // URL Firebase: Host + Path + .json + ?auth=Secret
  String url = "AT+HTTPPARA=\"URL\",\"https://";
  url += FIREBASE_HOST;
  url += FIREBASE_PATH;
  url += "?auth=";
  url += FIREBASE_AUTH;
  url += "\"";
  
  if (!sendAT(url.c_str(), 5000)) { sendAT("AT+HTTPTERM", 2000); return false; }
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 2000);
  
  // JSON Payload dengan Server Timestamp Firebase
  String json = "{\"device\":\"" + String(DEVICE_ID) + 
                "\",\"lat\":" + String(lat, 6) + 
                ",\"lng\":" + String(lng, 6) + 
                ",\"t\":{\".sv\":\"timestamp\"}}";
  
  String dataCmd = "AT+HTTPDATA=" + String(json.length()) + ",10000";
  sim900.println(dataCmd);
  delay(500);
  
  if (readResponse(3000).indexOf("DOWNLOAD") >= 0) {
    sim900.print(json);
    readResponse(5000);
  } else {
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  sim900.println("AT+HTTPACTION=1"); // 1 = POST
  
  bool success = false;
  unsigned long start = millis();
  while (millis() - start < 30000) {
    String resp = readResponse(1000);
    if (resp.indexOf("+HTTPACTION: 1,200") >= 0 || resp.indexOf("+HTTPACTION: 1,201") >= 0) {
      success = true;
      break;
    }
    if (resp.indexOf("+HTTPACTION: 1,") >= 0) break; 
  }
  
  sendAT("AT+HTTPTERM", 2000);
  return success;
}

// ===== SETUP =====
void setup() {
  pinMode(Vext_Ctrl, OUTPUT); digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT); digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  sim900.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  showStatus("BOOTING");

  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, LOW); delay(200); digitalWrite(GPRS_RST_PIN, HIGH);
  
  delay(5000); // Tunggu SIM900A stabil
  
  if (sendAT("AT", 2000)) {
    if (activateGPRS()) {
      gprsReady = true;
      showStatus("READY");
    } else {
      showStatus("GPRS ERR");
    }
  } else {
    showStatus("NO SIM");
  }
}

// ===== LOOP =====
void loop() {
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }
  
  static unsigned long lastDisp = 0;
  if (millis() - lastDisp >= 1000) {
    lastDisp = millis();
    updateDisplay();
  }
  
  static unsigned long lastSend = 0;
  // Kirim data setiap 30 detik jika GPS fix
  if (gprsReady && gps.location.isValid() && (millis() - lastSend >= 30000)) {
    lastSend = millis();
    showStatus("SENDING");
    if (sendToFirebase(gps.location.lat(), gps.location.lng())) {
      showStatus("SENT OK");
    } else {
      showStatus("SEND FAIL");
    }
    delay(2000);
  }
}

// ===== DISPLAY FUNCTIONS =====
void showStatus(const char* msg) {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(ST7735_BLACK);
  framebuffer.setTextSize(2);
  framebuffer.setCursor(10, 30);
  framebuffer.print(msg);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void updateDisplay() {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(5, 5);
  framebuffer.print("FIREBASE GPS TRACKER");
  
  if (gps.location.isValid()) {
    framebuffer.setCursor(5, 25);
    framebuffer.printf("LAT: %.6f", gps.location.lat());
    framebuffer.setCursor(5, 40);
    framebuffer.printf("LNG: %.6f", gps.location.lng());
    framebuffer.setCursor(5, 55);
    framebuffer.printf("SAT: %d", gps.satellites.value());
  } else {
    framebuffer.setCursor(40, 40);
    framebuffer.print("WAITING GPS...");
  }
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}