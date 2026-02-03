#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>


#define Vext_Ctrl 3    
#define LED_K     21   
#define TFT_CS    38
#define TFT_DC    40
#define TFT_RST   39
#define TFT_SCLK  41
#define TFT_MOSI  42
#define GPS_RX    33  
#define GPS_TX    34   

TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);  // 160x80 landscape

void setup() {
  
  // 1. Power on Vext (GPS + TFT)
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  
  // 2. AKTIFKAN BACKLIGHT TFT (INI YANG SERING DILUPAKAN!)
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);  // GPIO21 = backlight TFT v1.2
  
  // 3. LED built-in (opsional)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  delay(100);  // Delay 100ms seperti program dosen

  // 4. Serial untuk debugging & GPS
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // 5. Inisialisasi TFT (SAMA PERSIS program dosen)
  tft.initR(INITR_MINI160x80);  // 160x80 display
  tft.invertDisplay(false);     // Penting untuk v1.2!
  tft.setRotation(1);           // Landscape mode

  // 6. Clear display
  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  Serial.println("GPS + TFT Ready");
}

void loop() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Update tiap 1 detik
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

      // Serial output
      Serial.print(gps.location.lat(), 6);
      Serial.print(",");
      Serial.print(gps.location.lng(), 6);
      Serial.print(",");
      Serial.print(gps.satellites.value());
      Serial.print(",");
      Serial.println(gps.altitude.meters(), 1);
    } else {
      framebuffer.setCursor(25, 25);
      framebuffer.setTextColor(ST7735_RED, ST7735_BLACK);
      framebuffer.setTextSize(2);
      framebuffer.print("NO FIX");
      framebuffer.setTextSize(1);
      Serial.println("NO FIX");
    }

    // Push ke display
    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
  }
}