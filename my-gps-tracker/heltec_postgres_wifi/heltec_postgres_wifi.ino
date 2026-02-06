#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h> // Library untuk kirim data ke API Next.js

// --- KONFIGURASI PIN ---
#define Vext_Ctrl 3
#define LED_K 21
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42
#define GPS_RX 33
#define GPS_TX 34

// --- KONEKSI WIFI & SERVER LOCAL ---
#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""
// IP Laptop kamu sesuai terminal tadi
const char* serverName = "http://192.168.11.18:3000/api/track"; 

// Objek Global
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

void setup() {
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(100);

  Serial.begin(115200);
  // GPS Heltec Wireless Tracker biasanya pakai 9600 atau 115200
  Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); 

  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  // Koneksi WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");

  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  Serial.println("GPS + TFT + API Local Ready");
}

void loop() {
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 5000) { // Kirim tiap 5 detik agar server tidak berat
    lastUpdate = millis();

    framebuffer.fillScreen(ST7735_BLACK);
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setTextSize(1);
    framebuffer.setCursor(5, 5);
    framebuffer.print("GPS TO NEXTJS API");

    if (gps.location.isValid()) {
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      int sats = gps.satellites.value();

      // Update Tampilan TFT
      framebuffer.setCursor(5, 20);
      framebuffer.print("Lat:"); framebuffer.print(lat, 6);
      framebuffer.setCursor(5, 35);
      framebuffer.print("Lon:"); framebuffer.print(lng, 6);
      framebuffer.setCursor(5, 50);
      framebuffer.print("Sats:"); framebuffer.print(sats);
      framebuffer.print(" | WiFi: OK");

      // --- KIRIM DATA KE API NEXT.JS ---
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");

        // Format JSON sesuai dengan API route.ts kamu
        String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(lat, 6) + "\",\"lng\":\"" + String(lng, 6) + "\"}";
        
        int httpResponseCode = http.POST(jsonData);
        
        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);
        
        if(httpResponseCode > 0) {
           framebuffer.setCursor(5, 65);
           framebuffer.setTextColor(ST7735_GREEN, ST7735_BLACK);
           framebuffer.print("Server: SENT OK");
        } else {
           framebuffer.setCursor(5, 65);
           framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
           framebuffer.print("Server ERROR");
        }
        http.end();
      }

    } else {
      framebuffer.setCursor(25, 25);
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.setTextSize(2);
      framebuffer.print("NO FIX");
      Serial.println("Waiting for GPS...");
    }

    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }
}