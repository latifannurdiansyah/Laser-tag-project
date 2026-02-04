/*
  GPS Tracker with SIM900A and TFT Display (57600 baud) - Minimalist Black/White Edition
  Fully compilable version with proper forward declarations (no duplicate default arguments).
  Features automatic GPRS activation sequence after network registration with URC handling.

  The circuit:
    Heltec Wireless Tracker v1.2 pins:
    * Vext_Ctrl (GPIO3)  - Controls external power for GPS, TFT, and SIM900A module
    * LED_K (GPIO21)     - TFT backlight control (critical for v1.2 display visibility)
    * TFT_CS (GPIO38)    - ST7735 chip select
    * TFT_DC (GPIO40)    - ST7735 data/command select
    * TFT_RST (GPIO39)   - ST7735 reset pin
    * TFT_SCLK (GPIO41)  - SPI clock for TFT
    * TFT_MOSI (GPIO42)  - SPI MOSI for TFT
    * GPS_RX (GPIO33)    - UART RX for built-in GNSS receiver (115200 baud)
    * GPS_TX (GPIO34)    - UART TX for built-in GNSS receiver (115200 baud)
    * GPRS_TX_PIN (GPIO17) - ESP32 TX → SIM900A RX (UART2 @ 57600 baud)
    * GPRS_RX_PIN (GPIO16) - ESP32 RX ← SIM900A TX (UART2 @ 57600 baud)
    * GPRS_RST_PIN (GPIO15) - Hardware reset for SIM900A module

    External connections (CRITICAL FOR GPRS):
    * SIM900A VCC        - 5V external power supply (2A minimum recommended)
    * SIM900A GND        - Common ground with Heltec board (MANDATORY)
    * SIM900A Antenna    - U.FL/IPEX connector (MANDATORY)

  Created 03 February 2026
  Modified 03 February 2026
  By Muhammad Latif (Electrical Engineering Final Project)

  Academic purpose notice:
    This firmware implements automatic GPRS activation sequence after network registration
    with proper URC handling. Features minimalist black-on-white TFT display design.
    Fully compilable with no duplicate default arguments.
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// ===== PIN DEFINITIONS =====
#define Vext_Ctrl  3    // External power control (GPS + TFT + SIM900A)
#define LED_K      21   // TFT backlight control (Heltec v1.2 specific)
#define TFT_CS     38
#define TFT_DC     40
#define TFT_RST    39
#define TFT_SCLK   41
#define TFT_MOSI   42
#define GPS_RX     33   // Built-in GNSS UART RX (115200 baud)
#define GPS_TX     34   // Built-in GNSS UART TX (115200 baud)
#define GPRS_TX_PIN 17  // ESP32 TX → SIM900A RX
#define GPRS_RX_PIN 16  // ESP32 RX ← SIM900A TX
#define GPRS_RST_PIN 15 // SIM900A hardware reset

// ===== NETWORK CONFIGURATION =====
const char* APN = "indosatgprs";  // Sesuai screenshot HP Anda
const char* APN_USER = "";
const char* APN_PASS = "";

// ===== SUPABASE CONFIGURATION =====
const char* SUPABASE_URL = "https://rqdjcxqpamfhdkzlmxnw.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJxZGpjeHFwYW1maGRremxteG53Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzAwODAxMDAsImV4cCI6MjA4NTY1NjEwMH0.fXjGOUSHUKIq9yHkIVv_G3fGIIUBU1l6D2d7MCSAXlU";
const char* DEVICE_ID = "heltec-tracker-01";

// ===== GLOBAL OBJECTS =====
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);
HardwareSerial sim900(2);  // UART2 @ 57600 baud

// ===== STATE VARIABLES =====
bool gprsReady = false;
int lastSignalQuality = -1;

// ===== FORWARD DECLARATIONS (NO DUPLICATE DEFAULT ARGUMENTS) =====
void flushSIM900A(unsigned long timeout);
String readFullResponse(unsigned long timeout, bool echo);
bool sendATCommand(const char* cmd, unsigned long timeout);
bool activateGPRS();
bool sendToSupabase(double lat, double lng);
void updateDisplay();
void showStatus(const char* msg, uint16_t color);
void blinkLED(int interval, int count);

// ===== UTILITY FUNCTIONS =====
void flushSIM900A(unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout && sim900.available()) {
    sim900.read();  // Buang semua byte di buffer (termasuk URC)
  }
}

String readFullResponse(unsigned long timeout, bool echo) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (sim900.available()) {
      char c = sim900.read();
      // Abaikan URC yang dimulai dengan '*'
      if (c == '*' && response.length() == 0) {
        response += c;
        while (sim900.available()) {
          char d = sim900.read();
          response += d;
          if (d == '\n' || d == '\r') break;
        }
        if (echo) {
          Serial.print("[URC IGNORED] ");
          Serial.println(response);
        }
        response = "";  // Reset untuk respons berikutnya
        continue;
      }
      response += c;
      if (echo) Serial.write(c);
    }
    if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) break;
    delay(10);
  }
  if (echo && response.length() > 0 && response.charAt(response.length()-1) != '\n') {
    Serial.println();
  }
  return response;
}

bool sendATCommand(const char* cmd, unsigned long timeout) {
  Serial.print("→ ");
  Serial.println(cmd);
  sim900.println(cmd);
  delay(300);
  
  String response = readFullResponse(timeout, true);
  response.toLowerCase();
  return (response.indexOf("ok") >= 0);
}

// ===== GPRS ACTIVATION SEQUENCE =====
bool activateGPRS() {
  Serial.println("\n[DEBUG] Starting GPRS activation sequence...");
  
  // Langkah 1: Attach ke GPRS service
  Serial.println("[STEP 1] Attaching to GPRS service (AT+CGATT=1)...");
  if (!sendATCommand("AT+CGATT=1", 15000)) {
    Serial.println("[ERROR] Failed to attach to GPRS");
    return false;
  }
  Serial.println("[OK] Attached to GPRS service");

  // Langkah 2: Set APN
  Serial.println("[STEP 2] Setting APN to 'indosatgprs'...");
  String apnCmd = "AT+CSTT=\"";
  apnCmd += APN;
  apnCmd += "\",\"";
  apnCmd += APN_USER;
  apnCmd += "\",\"";
  apnCmd += APN_PASS;
  apnCmd += "\"";
  if (!sendATCommand(apnCmd.c_str(), 8000)) {
    Serial.println("[ERROR] Failed to set APN");
    return false;
  }
  Serial.println("[OK] APN configured");

  // Langkah 3: Activate PDP context
  Serial.println("[STEP 3] Activating wireless connection (AT+CIICR)...");
  if (!sendATCommand("AT+CIICR", 25000)) {
    Serial.println("[ERROR] Failed to activate wireless connection");
    return false;
  }
  Serial.println("[OK] Wireless connection activated");

  // Langkah 4: Dapatkan IP address
  Serial.println("[STEP 4] Requesting IP address (AT+CIFSR)...");
  sim900.println("AT+CIFSR");
  delay(1000);
  
  String response = readFullResponse(10000, true);
  if (response.indexOf(".") >= 0) {  // IP address contains dot
    Serial.println("[OK] IP address obtained - GPRS READY!");
    return true;
  } else {
    Serial.println("[ERROR] Failed to obtain IP address");
    return false;
  }
}

// ===== SETUP FUNCTION =====
void setup() {
  // Power initialization
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);

  // Serial interfaces
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
  sim900.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);

  // TFT initialization (MINIMALIST BLACK/WHITE)
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  framebuffer.fillScreen(ST7735_WHITE);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  Serial.println("=== SYSTEM INITIALIZATION ===");

  // SIM900A reset
  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, LOW);
  delay(200);
  digitalWrite(GPRS_RST_PIN, HIGH);
  Serial.println("[DEBUG] Waiting 6 seconds for SIM900A boot...");
  for (int i = 6; i > 0; i--) {
    Serial.print(i); Serial.print(" ");
    delay(1000);
  }
  Serial.println();

  // Flush boot messages (termasuk URC *PSNWID, *PSUTTZ)
  flushSIM900A(3000);

  // Verifikasi koneksi dasar
  if (sendATCommand("AT", 2000)) {
    Serial.println("[OK] SIM900A responding");
    
    // Cek status SIM
    sim900.println("AT+CPIN?");
    delay(1000);
    String cpiResponse = readFullResponse(2000, true);
    if (cpiResponse.indexOf("READY") >= 0) {
      Serial.println("[OK] SIM card ready");
      
      // Cek registrasi jaringan
      sim900.println("AT+COPS?");
      delay(2000);
      String copsResponse = readFullResponse(3000, true);
      if (copsResponse.indexOf("INDOSAT") >= 0) {
        Serial.println("[OK] Registered to Indosat network");
        
        // AKTIVASI GPRS (KRITIS!)
        if (activateGPRS()) {
          gprsReady = true;
          // Dapatkan sinyal
          sim900.println("AT+CSQ");
          delay(1000);
          String csqResponse = readFullResponse(2000, false);
          if (csqResponse.indexOf("+CSQ:") >= 0) {
            int csqPos = csqResponse.indexOf("+CSQ: ") + 6;
            int commaPos = csqResponse.indexOf(",", csqPos);
            if (commaPos > csqPos) {
              lastSignalQuality = csqResponse.substring(csqPos, commaPos).toInt();
              Serial.print("[SIGNAL] Quality: ");
              Serial.print(lastSignalQuality);
              Serial.println("/31");
            }
          }
          showStatus("GPRS READY", ST7735_BLACK);
          delay(2000);
        } else {
          Serial.println("[WARNING] GPRS activation failed - GPS only mode");
          showStatus("GPS ONLY", ST7735_BLACK);
          delay(2000);
        }
      } else {
        Serial.println("[ERROR] Not registered to Indosat network");
        showStatus("NO SIGNAL", ST7735_BLACK);
        delay(2000);
      }
    } else {
      Serial.println("[ERROR] SIM card not ready (locked or missing)");
      showStatus("SIM ERROR", ST7735_BLACK);
      delay(2000);
    }
  } else {
    Serial.println("[ERROR] SIM900A not responding");
    showStatus("SIM ERROR", ST7735_BLACK);
    delay(2000);
  }

  Serial.println("\n=== SYSTEM READY ===");
}

// ===== LOOP FUNCTION =====
void loop() {
  // Proses GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Update display tiap detik
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    updateDisplay();
  }

  // Kirim data tiap 60 detik jika GPS fix dan GPRS ready
  static unsigned long lastSend = 0;
  if (gprsReady && gps.location.isValid() && millis() - lastSend >= 60000) {
    lastSend = millis();
    
    // Kirim ke Supabase
    showStatus("SENDING", ST7735_BLACK);
    if (sendToSupabase(gps.location.lat(), gps.location.lng())) {
      showStatus("SENT", ST7735_BLACK);
      blinkLED(100, 3);
      Serial.print("[OK] Data sent: ");
      Serial.print(gps.location.lat(), 6);
      Serial.print(", ");
      Serial.println(gps.location.lng(), 6);
    } else {
      showStatus("FAILED", ST7735_BLACK);
      blinkLED(500, 2);
      Serial.println("[ERROR] Transmission failed");
    }
    delay(2000);
  }
}

// ===== SUPABASE TRANSMISSION =====
bool sendToSupabase(double lat, double lng) {
  // Flush buffer sebelum HTTP
  flushSIM900A(1000);
  delay(500);

  // HTTP init
  if (!sendATCommand("AT+HTTPINIT", 5000)) return false;
  
  // Set parameters
  if (!sendATCommand("AT+HTTPPARA=\"CID\",1", 3000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  String urlCmd = "AT+HTTPPARA=\"URL\",\"";
  urlCmd += SUPABASE_URL;
  urlCmd += "/rest/v1/locations\"";
  if (!sendATCommand(urlCmd.c_str(), 5000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  if (!sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 3000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  String keyCmd = "AT+HTTPPARA=\"USERDATA\",\"apikey: ";
  keyCmd += SUPABASE_KEY;
  keyCmd += "\"";
  if (!sendATCommand(keyCmd.c_str(), 3000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }

  // Kirim data
  String json = "{\"device_id\":\"";
  json += DEVICE_ID;
  json += "\",\"latitude\":";
  json += String(lat, 6);
  json += ",\"longitude\":";
  json += String(lng, 6);
  json += "}";
  
  String dataCmd = "AT+HTTPDATA=";
  dataCmd += String(json.length());
  dataCmd += ",10000";
  if (!sendATCommand(dataCmd.c_str(), 10000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  sim900.print(json);
  delay(3000);

  // Execute POST
  unsigned long start = millis();
  bool success = false;
  while (millis() - start < 35000) {
    sim900.println("AT+HTTPACTION=1");
    delay(1000);
    
    String response = readFullResponse(5000, false);
    if (response.indexOf("+HTTPACTION: 1,200") >= 0) {
      success = true;
      break;
    }
    if (response.indexOf("+HTTPACTION: 1,6") >= 0) {
      Serial.println("[ERROR] HTTP 603 - DNS resolution failed");
      break;
    }
    delay(1000);
  }
  
  sendATCommand("AT+HTTPREAD", 3000);
  sendATCommand("AT+HTTPTERM", 3000);
  
  return success;
}

// ===== DISPLAY FUNCTIONS (MINIMALIST BLACK/WHITE) =====
void showStatus(const char* msg, uint16_t color) {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(color, ST7735_WHITE);
  framebuffer.setTextSize(2);
  framebuffer.setCursor(10, 30);
  framebuffer.print(msg);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void blinkLED(int interval, int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(interval);
    digitalWrite(LED_BUILTIN, LOW);
    if (i < count - 1) delay(interval);
  }
}

void updateDisplay() {
  framebuffer.fillScreen(ST7735_WHITE);
  framebuffer.setTextColor(ST7735_BLACK, ST7735_WHITE);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(5, 5);
  framebuffer.print("GPS TRACKER");

  if (gps.location.isValid()) {
    framebuffer.setCursor(5, 20);
    framebuffer.print("Lat:");
    framebuffer.print(gps.location.lat(), 5);

    framebuffer.setCursor(5, 35);
    framebuffer.print("Lon:");
    framebuffer.print(gps.location.lng(), 5);

    framebuffer.setCursor(5, 50);
    framebuffer.print("Sats:");
    framebuffer.print(gps.satellites.value());
    framebuffer.print(" Alt:");
    framebuffer.print((int)gps.altitude.meters());
    framebuffer.print("m");

    framebuffer.setCursor(5, 65);
    framebuffer.print("GPRS:");
    if (!gprsReady) {
      framebuffer.print("OFFLINE");
    } else if (lastSignalQuality >= 0 && lastSignalQuality < 5) {
      framebuffer.print("WEAK");
    } else {
      framebuffer.print("OK");
    }

    Serial.print("FIX | ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(", ");
    Serial.print(gps.location.lng(), 6);
    Serial.print(" | Sats: ");
    Serial.print(gps.satellites.value());
    if (gprsReady && lastSignalQuality >= 0) {
      Serial.print(" | Sig: ");
      Serial.print(lastSignalQuality);
    }
    Serial.println();
  } else {
    framebuffer.setCursor(25, 25);
    framebuffer.setTextSize(2);
    framebuffer.print("NO FIX");
    framebuffer.setTextSize(1);
    framebuffer.setCursor(15, 45);
    framebuffer.print("Outdoors for");
    framebuffer.setCursor(25, 55);
    framebuffer.print("GPS signal");
    Serial.println("NO FIX - Move outdoors");
  }

  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}