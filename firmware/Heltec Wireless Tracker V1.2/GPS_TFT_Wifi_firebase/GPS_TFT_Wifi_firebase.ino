#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>

// --- LIBRARY TAMBAHAN UNTUK FIREBASE & WIFI ---
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// --- KONFIGURASI PIN (Tetap Sama) ---
#define Vext_Ctrl 3    
#define LED_K     21   
#define TFT_CS    38
#define TFT_DC    40
#define TFT_RST   39
#define TFT_SCLK  41
#define TFT_MOSI  42
#define GPS_RX    33  
#define GPS_TX    34  

// --- KREDENSIAL WIFI & FIREBASE (SESUAIKAN DI SINI) ---
#define WIFI_SSID "UserAndroid:"
#define WIFI_PASSWORD "55555550"
#define FIREBASE_HOST "gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AMTJsG2px3hwlumdLkvcpnFNQF3hybkw0WvwM9WI"

// Objek Global
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  // 1. Power on Vext (GPS + TFT)
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  
  // 2. Aktifkan Backlight TFT
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH); 
  
  // 3. LED built-in
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  delay(100);

  // 4. Serial Debugging & GPS
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // 5. Inisialisasi TFT
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  // 6. Koneksi WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");

  // 7. Konfigurasi Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // 8. Clear display awal
  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  Serial.println("GPS + TFT + Firebase Ready");
}

void loop() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Update tiap 1 detik (Gunakan interval agar tidak spam Firebase)
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    // Render ke framebuffer
    framebuffer.fillScreen(ST7735_BLACK);
    framebuffer.setTextColor(ST7735_WHITE, ST7735_BLACK);
    framebuffer.setTextSize(1);
    framebuffer.setCursor(5, 5);
    framebuffer.print("GPS STATUS");

    if (gps.location.isValid()) {
      float lat = gps.location.lat();
      float lng = gps.location.lng();
      int sats = gps.satellites.value();
      int alt = (int)gps.altitude.meters();

      // Update Tampilan TFT
      framebuffer.setCursor(5, 20);
      framebuffer.print("Lat:"); framebuffer.print(lat, 5);
      framebuffer.setCursor(5, 35);
      framebuffer.print("Lon:"); framebuffer.print(lng, 5);
      framebuffer.setCursor(5, 50);
      framebuffer.print("Sats:"); framebuffer.print(sats);
      framebuffer.print(" Alt:"); framebuffer.print(alt); framebuffer.print("m");

      // Kirim ke Firebase (Hanya jika data valid)
      FirebaseJson json;
      json.set("latitude", lat);
      json.set("longitude", lng);
      json.set("satellites", sats);
      json.set("altitude", alt);
      
      if (Firebase.ready()) {
        if (Firebase.RTDB.setJSON(&fbdo, "/gps_tracking", &json)) {
           Serial.println("Firebase: Data Sent");
        } else {
           Serial.println("Firebase Error: " + fbdo.errorReason());
        }
      }

      // Serial Output
      Serial.printf("Lat: %.6f, Lon: %.6f, Sats: %d\n", lat, lng, sats);
      
    } else {
      framebuffer.setCursor(25, 25);
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.setTextSize(2);
      framebuffer.print("NO FIX");
      framebuffer.setTextSize(1);
      Serial.println("Searching for GPS Fix...");
    }

    // Push ke display fisik
    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }
}