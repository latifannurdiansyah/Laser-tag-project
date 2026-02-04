/*
  GPS Tracker - ULTRA SIMPLIFIED VERSION
  Minimal HTTP setup for maximum compatibility with SIM900A
  
  This version:
  - Uses POST without custom headers (relies on URL apikey only)
  - Minimal error checking for debugging
  - Verbose serial output
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
const char* SUPABASE_URL = "https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJxZGpjeHFwYW1maGRremxteG53Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzcwMzMzOTksImV4cCI6MjA1MjYwOTM5OX0.M_z7fMJ2E-i4-r93LQoLlqWYMDaKbVxVd19aQs7r46s";
const char* DEVICE_ID = "heltec-tracker-01";

// ===== GLOBAL OBJECTS =====
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);
HardwareSerial sim900(2);

bool gprsReady = false;
int lastSignalQuality = -1;

// ===== FORWARD DECLARATIONS =====
void flushSIM900A(unsigned long timeout);
String readResponse(unsigned long timeout);
bool sendAT(const char* cmd, unsigned long timeout);
bool activateGPRS();
bool sendToSupabase(double lat, double lng);
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
  delay(100);
  String response = readResponse(timeout);
  return (response.indexOf("OK") >= 0);
}

// ===== GPRS ACTIVATION =====
bool activateGPRS() {
  Serial.println("\n[GPRS] Activating...");
  
  sendAT("AT+CIPSHUT", 5000);
  delay(1000);
  
  if (!sendAT("AT+CGATT=1", 15000)) {
    Serial.println("[ERROR] CGATT failed");
    return false;
  }
  
  delay(2000);
  
  String apnCmd = "AT+CSTT=\"" + String(APN) + "\",\"\",\"\"";
  if (!sendAT(apnCmd.c_str(), 8000)) {
    Serial.println("[ERROR] CSTT failed");
    return false;
  }
  
  if (!sendAT("AT+CIICR", 25000)) {
    Serial.println("[ERROR] CIICR failed");
    return false;
  }
  
  sim900.println("AT+CIFSR");
  delay(1000);
  String response = readResponse(10000);
  
  if (response.indexOf(".") >= 0) {
    Serial.println("[OK] GPRS ready");
    return true;
  }
  
  Serial.println("[ERROR] No IP");
  return false;
}

// ===== SETUP =====
void setup() {
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  sim900.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  
  Serial.println("\n=== GPS TRACKER v2 ===");
  
  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, LOW);
  delay(200);
  digitalWrite(GPRS_RST_PIN, HIGH);
  
  showStatus("BOOT");
  for (int i = 6; i > 0; i--) {
    Serial.print(i); Serial.print(" ");
    delay(1000);
  }
  Serial.println();
  
  flushSIM900A(3000);
  
  if (sendAT("AT", 2000)) {
    Serial.println("[OK] SIM900A OK");
    
    sim900.println("AT+CPIN?");
    delay(1000);
    String cpin = readResponse(2000);
    
    if (cpin.indexOf("READY") >= 0) {
      Serial.println("[OK] SIM ready");
      
      sim900.println("AT+COPS?");
      delay(2000);
      String cops = readResponse(3000);
      
      if (cops.indexOf("INDOSAT") >= 0 || cops.indexOf("51089") >= 0) {
        Serial.println("[OK] Network OK");
        
        if (activateGPRS()) {
          gprsReady = true;
          showStatus("READY");
          digitalWrite(LED_BUILTIN, HIGH);
        } else {
          showStatus("GPS ONLY");
        }
      } else {
        Serial.println("[ERROR] No network");
        showStatus("NO NET");
      }
    } else {
      Serial.println("[ERROR] SIM error");
      showStatus("SIM ERR");
    }
  } else {
    Serial.println("[ERROR] SIM900A not responding");
    showStatus("SIM ERR");
  }
  
  delay(2000);
  Serial.println("=== READY ===\n");
}

// ===== LOOP =====
void loop() {
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    updateDisplay();
  }
  
  static unsigned long lastSend = 0;
  if (gprsReady && gps.location.isValid() && millis() - lastSend >= 60000) {
    lastSend = millis();
    
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    
    Serial.print("\n[SEND] ");
    Serial.print(lat, 6);
    Serial.print(", ");
    Serial.println(lng, 6);
    
    showStatus("SEND");
    
    if (sendToSupabase(lat, lng)) {
      showStatus("OK");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("[OK] Sent");
    } else {
      showStatus("FAIL");
      Serial.println("[ERROR] Failed");
    }
    
    delay(2000);
  }
}

// ===== SUPABASE SEND (SIMPLIFIED) =====
bool sendToSupabase(double lat, double lng) {
  Serial.println("\n[HTTP] Starting...");
  
  flushSIM900A(1000);
  delay(500);
  
  if (!sendAT("AT+HTTPINIT", 5000)) {
    Serial.println("[ERROR] HTTPINIT");
    return false;
  }
  
  if (!sendAT("AT+HTTPPARA=\"CID\",1", 3000)) {
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  // URL with apikey parameter (no custom headers needed!)
  String url = "AT+HTTPPARA=\"URL\",\"https://";
  url += SUPABASE_URL;
  url += "/rest/v1/locations?apikey=";
  url += SUPABASE_KEY;
  url += "\"";
  
  Serial.println("[HTTP] Setting URL...");
  if (!sendAT(url.c_str(), 5000)) {
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  if (!sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 3000)) {
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  // Prepare JSON
  String json = "{\"device_id\":\"";
  json += DEVICE_ID;
  json += "\",\"latitude\":";
  json += String(lat, 6);
  json += ",\"longitude\":";
  json += String(lng, 6);
  json += "}";
  
  Serial.print("[HTTP] JSON: ");
  Serial.println(json);
  Serial.print("[HTTP] Length: ");
  Serial.println(json.length());
  
  String dataCmd = "AT+HTTPDATA=";
  dataCmd += String(json.length());
  dataCmd += ",10000";
  
  Serial.println("[HTTP] Preparing data...");
  sim900.println(dataCmd);
  delay(500);
  
  String downloadResp = readResponse(3000);
  if (downloadResp.indexOf("DOWNLOAD") < 0) {
    Serial.println("[ERROR] No DOWNLOAD prompt");
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  Serial.println("[HTTP] Sending JSON...");
  sim900.print(json);
  delay(3000);
  
  String dataResp = readResponse(5000);
  if (dataResp.indexOf("OK") < 0) {
    Serial.println("[ERROR] Data send failed");
    sendAT("AT+HTTPTERM", 2000);
    return false;
  }
  
  Serial.println("[HTTP] Executing POST...");
  sim900.println("AT+HTTPACTION=1");
  delay(1000);
  
  bool success = false;
  unsigned long start = millis();
  
  while (millis() - start < 30000) {
    String response = readResponse(3000);
    
    if (response.indexOf("+HTTPACTION: 1,200") >= 0 ||
        response.indexOf("+HTTPACTION: 1,201") >= 0) {
      Serial.println("[OK] HTTP 200/201");
      success = true;
      break;
    }
    
    if (response.indexOf("+HTTPACTION: 1,4") >= 0) {
      Serial.println("[ERROR] HTTP 4xx");
      break;
    }
    
    if (response.indexOf("+HTTPACTION: 1,6") >= 0) {
      Serial.println("[ERROR] HTTP 6xx");
      break;
    }
    
    delay(1000);
  }
  
  delay(500);
  sendAT("AT+HTTPREAD", 5000);
  sendAT("AT+HTTPTERM", 3000);
  
  return success;
}

// ===== DISPLAY =====
void showStatus(const char* msg) {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(ST7735_BLACK);
  framebuffer.setTextSize(2);
  framebuffer.setCursor(20, 30);
  framebuffer.print(msg);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void updateDisplay() {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(5, 5);
  framebuffer.print("GPS TRACKER v2");
  
  if (gps.location.isValid()) {
    framebuffer.setCursor(5, 20);
    framebuffer.print("Lat: ");
    framebuffer.print(gps.location.lat(), 5);
    
    framebuffer.setCursor(5, 35);
    framebuffer.print("Lon: ");
    framebuffer.print(gps.location.lng(), 5);
    
    framebuffer.setCursor(5, 50);
    framebuffer.print("Sats: ");
    framebuffer.print(gps.satellites.value());
    
    framebuffer.setCursor(5, 65);
    if (gprsReady) {
      framebuffer.print("GPRS: OK");
    } else {
      framebuffer.print("GPRS: OFF");
    }
  } else {
    framebuffer.setCursor(30, 30);
    framebuffer.setTextSize(2);
    framebuffer.print("NO FIX");
  }
  
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}
