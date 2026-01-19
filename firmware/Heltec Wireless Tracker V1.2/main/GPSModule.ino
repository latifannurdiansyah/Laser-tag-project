#include "PinConfig.h"

// Global object TinyGPSPlus
TinyGPSPlus gps;

// Variabel global untuk akses di seluruh sistem
float currentLat = 0.0;
float currentLon = 0.0;
float currentAlt = 0.0;
uint8_t currentSats = 0;
bool gpsValid = false;

uint8_t currentHour = 0;
uint8_t currentMinute = 0;
uint8_t currentSecond = 0;
bool timeValid = false;

void gpsFeed() {
  // Baca semua byte yang tersedia dari Serial1 (sesuai program dosen)
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    gps.encode(c); // Proses karakter satu per satu
  }

  // Ambil status validitas
  gpsValid = gps.location.isValid();
  timeValid = gps.time.isValid();

  // Update data hanya jika valid
  if (gpsValid) {
    currentLat = gps.location.lat();
    currentLon = gps.location.lng();
    currentAlt = gps.altitude.meters();
    currentSats = gps.satellites.value();
  }

  if (timeValid) {
    currentHour = gps.time.hour();
    currentMinute = gps.time.minute();
    currentSecond = gps.time.second();
  }
}