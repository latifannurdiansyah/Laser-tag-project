/*
  GPS Tracker with SIM900A and TFT Display - REVISED FOR SUPABASE
  Fixed HTTP headers and added proper Supabase authentication
  
  Changes from original:
  1. Fixed HTTP headers (added Authorization + Prefer headers)
  2. Improved error handling
  3. Better URC filtering
  4. Added connection retry logic
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

// ===== NETWORK CONFIGURATION =====
const char* APN = "indosatgprs";
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
HardwareSerial sim900(2);

// ===== STATE VARIABLES =====
bool gprsReady = false;
int lastSignalQuality = -1;
unsigned long lastGPRSCheck = 0;

// ===== FORWARD DECLARATIONS =====
void flushSIM900A(unsigned long timeout);
String readFullResponse(unsigned long timeout, bool echo);
bool sendATCommand(const char* cmd, unsigned long timeout);
bool activateGPRS();
bool checkGPRSConnection();
bool sendToSupabase(double lat, double lng);
void updateDisplay();
void showStatus(const char* msg, uint16_t color);
void blinkLED(int interval, int count);

// ===== UTILITY FUNCTIONS =====
void flushSIM900A(unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (sim900.available()) {
      sim900.read();
    }
  }
}

String readFullResponse(unsigned long timeout, bool echo) {
  String response = "";
  unsigned long start = millis();
  bool skipURC = false;
  
  while (millis() - start < timeout) {
    while (sim900.available()) {
      char c = sim900.read();
      
      // Skip URC lines (start with *)
      if (c == '*' && response.length() == 0) {
        skipURC = true;
      }
      
      if (skipURC) {
        if (echo) Serial.write(c);
        if (c == '\n') {
          skipURC = false;
          response = "";
        }
        continue;
      }
      
      response += c;
      if (echo) Serial.write(c);
    }
    
    // Check for completion
    if (response.indexOf("OK") >= 0 || 
        response.indexOf("ERROR") >= 0 ||
        response.indexOf("SEND OK") >= 0 ||
        response.indexOf("+HTTPACTION:") >= 0) {
      break;
    }
    
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
  
  flushSIM900A(100);
  sim900.println(cmd);
  delay(100);
  
  String response = readFullResponse(timeout, true);
  response.toLowerCase();
  
  bool success = (response.indexOf("ok") >= 0);
  if (!success && response.indexOf("error") >= 0) {
    Serial.println("[ERROR] Command failed");
  }
  
  return success;
}

// ===== GPRS CONNECTION CHECK =====
bool checkGPRSConnection() {
  sim900.println("AT+CIPSTATUS");
  delay(500);
  String response = readFullResponse(3000, false);
  
  // Status: IP INITIAL, IP START, IP CONFIG, IP GPRSACT, IP STATUS, TCP CLOSED, PDP DEACT
  if (response.indexOf("IP STATUS") >= 0 || 
      response.indexOf("IP GPRSACT") >= 0) {
    return true;
  }
  
  Serial.println("[DEBUG] GPRS connection lost, re-activating...");
  return activateGPRS();
}

// ===== GPRS ACTIVATION SEQUENCE =====
bool activateGPRS() {
  Serial.println("\n[DEBUG] Starting GPRS activation...");
  
  // Step 0: Reset GPRS context (clean state)
  Serial.println("[STEP 0] Resetting GPRS context...");
  sendATCommand("AT+CIPSHUT", 5000);  // Shutdown any existing connection
  delay(1000);
  
  // Step 1: Attach to GPRS
  Serial.println("[STEP 1] AT+CGATT=1");
  if (!sendATCommand("AT+CGATT=1", 15000)) {
    Serial.println("[ERROR] GPRS attach failed");
    return false;
  }
  
  // Wait and verify attach status
  Serial.println("[VERIFY] Checking GPRS attach status...");
  delay(2000);
  
  for (int retry = 0; retry < 5; retry++) {
    sim900.println("AT+CGATT?");
    delay(500);
    String response = readFullResponse(2000, true);
    
    if (response.indexOf("+CGATT: 1") >= 0) {
      Serial.println("[OK] GPRS attached");
      break;
    }
    
    if (retry == 4) {
      Serial.println("[ERROR] GPRS attach verification failed");
      return false;
    }
    
    Serial.print("[RETRY] Waiting for GPRS attach... ");
    Serial.println(retry + 1);
    delay(2000);
  }
  
  // Step 2: Set APN
  Serial.println("[STEP 2] Setting APN");
  delay(1000);  // Extra delay before APN setup
  
  String apnCmd = "AT+CSTT=\"" + String(APN) + "\",\"" + String(APN_USER) + "\",\"" + String(APN_PASS) + "\"";
  if (!sendATCommand(apnCmd.c_str(), 8000)) {
    Serial.println("[ERROR] APN setup failed");
    return false;
  }
  
  // Step 3: Activate connection
  Serial.println("[STEP 3] AT+CIICR");
  if (!sendATCommand("AT+CIICR", 25000)) {
    Serial.println("[ERROR] Connection activation failed");
    return false;
  }
  
  // Step 4: Get IP
  Serial.println("[STEP 4] AT+CIFSR");
  sim900.println("AT+CIFSR");
  delay(1000);
  String response = readFullResponse(10000, true);
  
  if (response.indexOf(".") >= 0) {
    Serial.println("[OK] GPRS READY!");
    return true;
  }
  
  Serial.println("[ERROR] No IP assigned");
  return false;
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

  // TFT initialization
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  framebuffer.fillScreen(ST7735_WHITE);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  Serial.println("\n=== GPS TRACKER INIT ===");

  // SIM900A reset
  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, LOW);
  delay(200);
  digitalWrite(GPRS_RST_PIN, HIGH);
  
  showStatus("BOOTING", ST7735_BLACK);
  Serial.println("[DEBUG] Waiting for SIM900A boot...");
  for (int i = 6; i > 0; i--) {
    Serial.print(i); Serial.print(" ");
    delay(1000);
  }
  Serial.println();

  // Flush boot messages
  flushSIM900A(3000);

  // Basic check
  if (sendATCommand("AT", 2000)) {
    Serial.println("[OK] SIM900A responding");
    
    // Check SIM
    sim900.println("AT+CPIN?");
    delay(1000);
    String cpin = readFullResponse(2000, true);
    
    if (cpin.indexOf("READY") >= 0) {
      Serial.println("[OK] SIM ready");
      
      // Check network
      sim900.println("AT+COPS?");
      delay(2000);
      String cops = readFullResponse(3000, true);
      
      if (cops.indexOf("INDOSAT") >= 0 || cops.indexOf("\"0\"") >= 0) {
        Serial.println("[OK] Network registered");
        
        // Activate GPRS
        if (activateGPRS()) {
          gprsReady = true;
          
          // Get signal
          sim900.println("AT+CSQ");
          delay(500);
          String csq = readFullResponse(2000, false);
          int csqPos = csq.indexOf("+CSQ: ");
          if (csqPos >= 0) {
            csqPos += 6;
            int comma = csq.indexOf(",", csqPos);
            if (comma > csqPos) {
              lastSignalQuality = csq.substring(csqPos, comma).toInt();
              Serial.print("[SIGNAL] ");
              Serial.print(lastSignalQuality);
              Serial.println("/31");
            }
          }
          
          showStatus("READY", ST7735_BLACK);
          blinkLED(200, 3);
        } else {
          Serial.println("[WARN] GPS only mode");
          showStatus("GPS ONLY", ST7735_BLACK);
        }
      } else {
        Serial.println("[ERROR] No network");
        showStatus("NO NET", ST7735_BLACK);
      }
    } else {
      Serial.println("[ERROR] SIM not ready");
      showStatus("SIM ERR", ST7735_BLACK);
    }
  } else {
    Serial.println("[ERROR] SIM900A not responding");
    showStatus("SIM ERR", ST7735_BLACK);
  }
  
  delay(2000);
  Serial.println("=== READY ===\n");
}

// ===== LOOP FUNCTION =====
void loop() {
  // Process GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Update display every second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    updateDisplay();
  }

  // Check GPRS every 5 minutes
  if (gprsReady && millis() - lastGPRSCheck >= 300000) {
    lastGPRSCheck = millis();
    gprsReady = checkGPRSConnection();
  }

  // Send data every 60 seconds
  static unsigned long lastSend = 0;
  if (gprsReady && gps.location.isValid() && millis() - lastSend >= 60000) {
    lastSend = millis();
    
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    
    Serial.print("\n[SEND] ");
    Serial.print(lat, 6);
    Serial.print(", ");
    Serial.println(lng, 6);
    
    showStatus("SENDING", ST7735_BLACK);
    
    if (sendToSupabase(lat, lng)) {
      showStatus("SENT OK", ST7735_BLACK);
      blinkLED(100, 3);
      Serial.println("[OK] Data transmitted");
    } else {
      showStatus("FAILED", ST7735_BLACK);
      blinkLED(500, 2);
      Serial.println("[ERROR] Transmission failed");
      // Re-check GPRS on next attempt
      lastGPRSCheck = 0;
    }
    
    delay(2000);
  }
}

// ===== SUPABASE TRANSMISSION (FIXED) =====
bool sendToSupabase(double lat, double lng) {
  Serial.println("\n[HTTP] Starting transmission...");
  
  // Flush buffer
  flushSIM900A(1000);
  delay(500);

  // HTTP init
  Serial.println("[HTTP] Init");
  if (!sendATCommand("AT+HTTPINIT", 5000)) {
    Serial.println("[ERROR] HTTP init failed");
    return false;
  }
  
  // Set CID
  if (!sendATCommand("AT+HTTPPARA=\"CID\",1", 3000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  // Set URL
  String url = String(SUPABASE_URL) + "/rest/v1/locations";
  String urlCmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
  Serial.print("[HTTP] URL: ");
  Serial.println(url);
  
  if (!sendATCommand(urlCmd.c_str(), 5000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  // Set Content-Type
  if (!sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 3000)) {
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  // CRITICAL FIX: Combine all headers into ONE USERDATA command
  // SIM900A only keeps the last USERDATA, so we must send all headers together
  String allHeaders = "apikey: " + String(SUPABASE_KEY) + "\\r\\n";
  allHeaders += "Authorization: Bearer " + String(SUPABASE_KEY) + "\\r\\n";
  allHeaders += "Prefer: return=representation";
  
  String headerCmd = "AT+HTTPPARA=\"USERDATA\",\"" + allHeaders + "\"";
  
  Serial.println("[HTTP] Setting headers:");
  Serial.println("  - apikey");
  Serial.println("  - Authorization Bearer");
  Serial.println("  - Prefer");
  
  if (!sendATCommand(headerCmd.c_str(), 3000)) {
    Serial.println("[ERROR] Failed to set headers");
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }

  // Prepare JSON
  String json = "{\"device_id\":\"" + String(DEVICE_ID) + 
                "\",\"latitude\":" + String(lat, 6) + 
                ",\"longitude\":" + String(lng, 6) + "}";
  
  Serial.print("[HTTP] JSON: ");
  Serial.println(json);
  
  // Set data length
  String dataCmd = "AT+HTTPDATA=" + String(json.length()) + ",10000";
  Serial.println("[HTTP] Preparing data...");
  
  sim900.println(dataCmd);
  delay(500);
  
  // Wait for DOWNLOAD prompt
  String downloadResp = readFullResponse(3000, true);
  if (downloadResp.indexOf("DOWNLOAD") < 0) {
    Serial.println("[ERROR] No DOWNLOAD prompt");
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }
  
  // Send JSON
  Serial.println("[HTTP] Sending JSON...");
  sim900.print(json);
  delay(3000);
  
  String dataResp = readFullResponse(5000, true);
  if (dataResp.indexOf("OK") < 0) {
    Serial.println("[ERROR] Data send failed");
    sendATCommand("AT+HTTPTERM", 2000);
    return false;
  }

  // Execute POST
  Serial.println("[HTTP] Executing POST...");
  sim900.println("AT+HTTPACTION=1");
  delay(1000);
  
  // Wait for response (max 30 seconds)
  bool success = false;
  unsigned long start = millis();
  
  while (millis() - start < 30000) {
    String response = readFullResponse(3000, true);
    
    if (response.indexOf("+HTTPACTION: 1,200") >= 0 ||
        response.indexOf("+HTTPACTION: 1,201") >= 0) {
      Serial.println("[OK] HTTP 200/201 - Success!");
      success = true;
      break;
    }
    
    if (response.indexOf("+HTTPACTION: 1,4") >= 0) {
      Serial.println("[ERROR] HTTP 4xx - Client error");
      break;
    }
    
    if (response.indexOf("+HTTPACTION: 1,6") >= 0) {
      Serial.println("[ERROR] HTTP 6xx - Network error");
      break;
    }
    
    delay(1000);
  }
  
  // Read response
  delay(500);
  sendATCommand("AT+HTTPREAD", 5000);
  
  // Terminate
  sendATCommand("AT+HTTPTERM", 3000);
  
  return success;
}

// ===== DISPLAY FUNCTIONS =====
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
      framebuffer.print("OFF");
    } else if (lastSignalQuality >= 0 && lastSignalQuality < 10) {
      framebuffer.print("WEAK");
    } else {
      framebuffer.print("OK");
    }

    // Serial log
    if (millis() % 5000 < 1000) {  // Only print every 5 seconds
      Serial.print("[GPS] ");
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
    }
  } else {
    framebuffer.setCursor(25, 25);
    framebuffer.setTextSize(2);
    framebuffer.print("NO FIX");
    framebuffer.setTextSize(1);
    framebuffer.setCursor(15, 45);
    framebuffer.print("Move to");
    framebuffer.setCursor(20, 55);
    framebuffer.print("outdoors");
  }

  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}
