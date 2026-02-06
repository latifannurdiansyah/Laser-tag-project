#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define Vext_Ctrl 3
#define LED_K 21
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42
#define GPS_RX 33
#define GPS_TX 34

#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""

// TODO: Ganti dengan URL Vercel Anda setelah deployment
// Contoh: const char* serverName = "https://gps-tracker-xyz.vercel.app/api/track";
const char* serverName = "https://your-vercel-app.vercel.app/api/track";

TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

void setup() {
  // 1. Power on Vext (GPS + TFT)
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  
  // 2. AKTIFKAN BACKLIGHT TFT
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  
  // 3. LED built-in
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  delay(100);

  // 4. Serial untuk debugging & GPS
  Serial.begin(115200);
  // PERBAIKAN UTAMA: Ubah baud rate dari 9600 ke 115200
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // 5. Inisialisasi TFT
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(false);
  tft.setRotation(1);

  // 6. Clear display
  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  // 7. Koneksi WiFi (setelah GPS siap)
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Tampilkan status WiFi di layar
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setCursor(5, 5);
  framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  framebuffer.setTextSize(1);
  framebuffer.print("Connecting WiFi...");
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    Serial.print(".");
    delay(500);
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed - continuing anyway");
  }

  Serial.println("GPS + TFT + API Local Ready");
}

void loop() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Update display setiap 5 detik (untuk API)
  // Tapi cek GPS lebih sering untuk debugging
  static unsigned long lastUpdate = 0;
  static unsigned long lastDebug = 0;
  
  // Debug GPS setiap 1 detik
  if (millis() - lastDebug >= 1000) {
    lastDebug = millis();
    Serial.print("Chars: ");
    Serial.print(gps.charsProcessed());
    Serial.print(" | Sentences: ");
    Serial.print(gps.sentencesWithFix());
    Serial.print(" | Failed: ");
    Serial.println(gps.failedChecksum());
  }
  
  if (millis() - lastUpdate >= 5000) {
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

      framebuffer.setCursor(5, 20);
      framebuffer.print("Lat:"); framebuffer.print(lat, 6);
      framebuffer.setCursor(5, 35);
      framebuffer.print("Lon:"); framebuffer.print(lng, 6);
      framebuffer.setCursor(5, 50);
      framebuffer.print("Sats:"); framebuffer.print(sats);

      // Serial output untuk debugging
      Serial.print("GPS: ");
      Serial.print(lat, 6);
      Serial.print(", ");
      Serial.println(lng, 6);

      if (WiFi.status() == WL_CONNECTED) {
        framebuffer.print(" | WiFi:OK");
        
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000); // Timeout 5 detik

        String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(lat, 6) + "\",\"lng\":\"" + String(lng, 6) + "\"}";

        Serial.print("Sending: ");
        Serial.println(jsonData);
        
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
      } else {
        framebuffer.print(" | WiFi:X");
        framebuffer.setCursor(5, 65);
        framebuffer.setTextColor(ST7735_YELLOW, ST7735_BLACK);
        framebuffer.print("WiFi Disconnected");
      }

    } else {
      framebuffer.setCursor(25, 25);
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.setTextSize(2);
      framebuffer.print("NO FIX");
      framebuffer.setTextSize(1);
      Serial.println("Waiting for GPS fix...");
    }

    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }
}