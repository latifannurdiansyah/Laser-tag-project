#include "PinConfig.h"

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void tftInit() {
  pinMode(TFT_LED_K, OUTPUT);
  digitalWrite(TFT_LED_K, HIGH);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("GPS+LoRa Tracker");
  tft.drawFastHLine(0, 10, 160, ST7735_CYAN);
}

void tftShowStatus(const char* line1, const char* line2, const char* line3) {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  
  tft.setCursor(0, 20); tft.print(line1);
  if (strlen(line2) > 0) { tft.setCursor(0, 35); tft.print(line2); }
  if (strlen(line3) > 0) { tft.setCursor(0, 50); tft.print(line3); }
}

void tftUpdateData(uint16_t irAddr, uint8_t irCmd, float lat, float lon, uint8_t sats) {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  tft.setTextSize(1);
  
  // === BARIS 1: WAKTU LOKAL ===
  uint8_t utcHour = 0, utcMin = 0, utcSec = 0;
  getGpsTime(&utcHour, &utcMin, &utcSec);
  
  int utcOffset = getUtcOffsetFromLongitude(lon);
  uint8_t localHour = (utcHour + utcOffset + 24) % 24;
  uint8_t localMin = utcMin;
  uint8_t localSec = utcSec;
  
  bool hasTime = isGpsTimeValid();
  tft.setTextColor(hasTime ? ST7735_GREEN : ST7735_BLUE);
  tft.setCursor(0, 15);
  if (hasTime) {
    tft.printf("%02d:%02d:%02d UTC%+d", localHour, localMin, localSec, utcOffset);
  } else {
    tft.print("Time: --:--:--");
  }
  
  // === BARIS 2: DATA IR ===
  tft.setTextColor(ST7735_YELLOW);
  tft.setCursor(0, 27);
  if (irAddr > 0) {
    tft.printf("IR: 0x%04X/0x%02X", irAddr, irCmd);
  } else {
    tft.print("IR: Waiting...");
  }
  
  // === BARIS 3: STATUS GPS ===
  tft.setTextColor(sats >= 4 ? ST7735_CYAN : ST7735_RED);
  tft.setCursor(0, 39);
  tft.printf("GPS: %d sats", sats);
  
  // === BARIS 4-5: KOORDINAT ===
  if (sats >= 4 && isGpsValid()) {
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(0, 51); 
    tft.printf("%.6f", lat);
    tft.setCursor(0, 61); 
    tft.printf("%.6f", lon);
  } else {
    tft.setTextColor(ST7735_RED);
    tft.setCursor(0, 51);
    tft.print("No GPS Fix");
  }
  
  // === BARIS 6: STATISTIK TX ===
  tft.setTextColor(ST7735_MAGENTA);
  tft.setCursor(0, 72);
  tft.printf("TX:%u E:%u", txCount, txErrors);
}