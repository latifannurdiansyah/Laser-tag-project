#include "PinConfig.h"

TinyGPSPlus gps;
float currentLat = 0.0;
float currentLon = 0.0;
float currentAlt = 0.0;
uint8_t currentSats = 0;
bool gpsValid = false;

void gpsFeed() {
  while (GPS_SERIAL.available() > 0) {
    char c = GPS_SERIAL.read();
    gps.encode(c);
    
    if (gps.location.isUpdated()) {
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentAlt = gps.altitude.meters();
      currentSats = gps.satellites.value();
      gpsValid = gps.location.isValid();
    }
  }
}

uint16_t readBatteryVoltage() {
  uint16_t adc = analogRead(BATTERY_ADC_PIN);
  return (uint16_t)((adc * BATTERY_VREF * BATTERY_ADC_SCALE) / 4095);
}