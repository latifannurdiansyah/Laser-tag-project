#include "PinConfig.h"

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void tftInit() {
  pinMode(TFT_LED_K, OUTPUT);
  digitalWrite(TFT_LED_K, HIGH);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setTextSize(1);
}

void tftShowStatus(const char* line1, const char* line2, const char* line3) {
  tft.fillRect(0, 0, 160, 80, ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setCursor(0, 5); tft.print(line1);
  if (line2 && strlen(line2)) { tft.setCursor(0, 20); tft.print(line2); }
  if (line3 && strlen(line3)) { tft.setCursor(0, 35); tft.print(line3); }
}

void tftUpdateData(
  uint16_t irAddr,
  uint8_t irCmd,
  float lat,
  float lon,
  uint8_t sats,
  uint8_t hour,
  uint8_t minute,
  uint8_t second,
  bool validTime,
  const String& loraStatus
) {
  tft.fillRect(0, 0, 160, 80, ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setTextSize(1);

  if (irAddr > 0) {
    char irBuf[24];
    snprintf(irBuf, sizeof(irBuf), "IR:0x%04X/0x%02X", irAddr, irCmd);
    tft.setCursor(0, 5); tft.print(irBuf);
  } else {
    tft.setCursor(0, 5); tft.print("IR: Waiting...");
  }

  if (gpsValid && sats >= 4) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Lat:%.5f", lat);
    tft.setCursor(0, 18); tft.print(buf);
    snprintf(buf, sizeof(buf), "Lon:%.5f", lon);
    tft.setCursor(0, 31); tft.print(buf);
  } else {
    tft.setCursor(0, 18); tft.print("Lat: ---");
    tft.setCursor(0, 31); tft.print("Lon: ---");
  }

  tft.setCursor(0, 44);
  if (validTime) {
    int8_t h = hour + TIMEZONE_OFFSET_HOURS;
    if (h >= 24) h -= 24; else if (h < 0) h += 24;
    tft.printf("Sat:%d | %02d:%02d:%02d", sats, h, minute, second);
  } else {
    tft.printf("Sat:%d | --:--:--", sats);
  }

  tft.setCursor(0, 57);
  tft.print("LoRa: ");
  if (loraStatus == "Joined") {
    tft.print("OK");
  } else if (loraStatus == "Joining") {
    tft.print("Joining...");
  } else {
    tft.print("JoinFailed");
  }

  tft.setCursor(0, 70);
  if (irAddr > 0) {
    tft.print(">>> HIT! <<<");
  } else {
    tft.print("Status: Standby");
  }
}