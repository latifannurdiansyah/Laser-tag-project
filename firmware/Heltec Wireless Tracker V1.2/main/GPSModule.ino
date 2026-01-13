#include "PinConfig.h"

TinyGPSPlus gps;
float currentLat = 0.0;
float currentLon = 0.0;
float currentAlt = 0.0;
uint8_t currentSats = 0;
bool gpsValid = false;
bool gpsTimeValid = false;

static unsigned long lastGpsUpdate = 0;
static unsigned long gpsStartTime = 0;
static bool gpsInitialized = false;
static unsigned long lastStatusPrint = 0;
static unsigned long lastDataWarning = 0;
static uint8_t lastSatsReported = 0;
static bool firstFixAcquired = false;
static bool nmeaDebugShown = false;

void gpsInit() {
  Serial.println(F("[GPS] =========================================="));
  Serial.println(F("[GPS] Initializing GPS module..."));
  
  // CRITICAL: Pastikan VEXT sudah HIGH
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(100);
  Serial.println(F("[GPS] VEXT_CTRL set to HIGH"));
  
  // Hard reset GPS module dengan timing yang lebih lama
  pinMode(GNSS_RST, OUTPUT);
  Serial.println(F("[GPS] Performing hard reset..."));
  digitalWrite(GNSS_RST, LOW);
  delay(500);  // Lebih lama untuk reset yang proper
  digitalWrite(GNSS_RST, HIGH);
  delay(1000); // Tunggu module siap
  
  // Start GPS serial communication
  GPS_SERIAL.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  GPS_SERIAL.flush();
  
  // Clear any old data
  while (GPS_SERIAL.available() > 0) {
    GPS_SERIAL.read();
  }
  
  gpsStartTime = millis();
  gpsInitialized = true;
  firstFixAcquired = false;
  lastSatsReported = 0;
  nmeaDebugShown = false;
  gpsTimeValid = false;
  
  Serial.println(F("[GPS] Module reset complete"));
  Serial.println(F("[GPS] Serial port opened at 9600 baud"));
  Serial.printf("[GPS] RX Pin: %d, TX Pin: %d\n", GPS_RX_PIN, GPS_TX_PIN);
  Serial.println(F("[GPS] =========================================="));
  Serial.println(F("[GPS] Cold start: 30-60 seconds"));
  Serial.println(F("[GPS] Warm start: 10-30 seconds"));
  Serial.println(F("[GPS] Hot start:  1-5 seconds"));
  Serial.println(F("[GPS] =========================================="));
  Serial.println(F(""));
}

void gpsFeed() {
  if (!gpsInitialized) {
    gpsInit();
  }
  
  // Tampilkan raw NMEA data untuk debugging (hanya sekali)
  if (!nmeaDebugShown && gps.charsProcessed() > 100 && gps.charsProcessed() < 500) {
    Serial.println(F(""));
    Serial.println(F("[GPS] ===== RAW NMEA DATA SAMPLE ====="));
    
    unsigned long startDebug = millis();
    while (millis() - startDebug < 2000 && GPS_SERIAL.available() > 0) {
      char c = GPS_SERIAL.read();
      Serial.write(c);
      gps.encode(c);
    }
    
    Serial.println(F("[GPS] ===== END NMEA SAMPLE ====="));
    Serial.println(F(""));
    nmeaDebugShown = true;
  }
  
  // Baca dan proses semua data GPS yang tersedia
  while (GPS_SERIAL.available() > 0) {
    char c = GPS_SERIAL.read();
    
    // Encode karakter ke TinyGPS++
    if (gps.encode(c)) {
      // Update location jika ada data baru
      if (gps.location.isUpdated()) {
        currentLat = gps.location.lat();
        currentLon = gps.location.lng();
        gpsValid = gps.location.isValid();
        lastGpsUpdate = millis();
        
        if (!firstFixAcquired && gpsValid) {
          unsigned long timeToFix = (millis() - gpsStartTime) / 1000;
          Serial.println(F(""));
          Serial.println(F("========================================"));
          Serial.printf("[GPS] FIRST FIX ACQUIRED in %lu seconds!\n", timeToFix);
          Serial.printf("[GPS] Position: %.6f, %.6f\n", currentLat, currentLon);
          Serial.println(F("========================================"));
          firstFixAcquired = true;
        }
      }
      
      // Update waktu GPS
      if (gps.time.isUpdated()) {
        gpsTimeValid = gps.time.isValid();
        if (gpsTimeValid && !firstFixAcquired) {
          Serial.printf("[GPS] Time available: %02d:%02d:%02d UTC\n", 
                        gps.time.hour(), gps.time.minute(), gps.time.second());
        }
      }
      
      if (gps.altitude.isUpdated()) {
        currentAlt = gps.altitude.meters();
      }
      
      if (gps.satellites.isUpdated()) {
        currentSats = gps.satellites.value();
        
        if (currentSats != lastSatsReported) {
          Serial.printf("[GPS] Satellites in view: %d\n", currentSats);
          lastSatsReported = currentSats;
          
          if (currentSats >= 4 && !gpsValid) {
            Serial.println(F("[GPS] Sufficient satellites, calculating position..."));
          } else if (currentSats > 0 && currentSats < 4) {
            Serial.printf("[GPS] Need %d more satellite(s) for fix\n", 4 - currentSats);
          }
        }
      }
      
      // Check HDOP (Horizontal Dilution of Precision)
      if (gps.hdop.isUpdated()) {
        double hdop = gps.hdop.hdop();
        if (hdop < 999.0) {
          Serial.printf("[GPS] HDOP: %.2f ", hdop);
          if (hdop < 2.0) Serial.println("(Excellent)");
          else if (hdop < 5.0) Serial.println("(Good)");
          else if (hdop < 10.0) Serial.println("(Moderate)");
          else Serial.println("(Poor)");
        }
      }
    }
  }
  
  // Status report setiap 10 detik
  if (millis() - lastStatusPrint >= 10000) {
    lastStatusPrint = millis();
    
    Serial.println(F(""));
    Serial.println(F("[GPS] ----- Status Report -----"));
    
    if (gpsValid) {
      Serial.printf("[GPS] VALID FIX\n");
      Serial.printf("[GPS] Lat: %.6f, Lon: %.6f\n", currentLat, currentLon);
      Serial.printf("[GPS] Alt: %.1fm, Sats: %d\n", currentAlt, currentSats);
      
      if (gpsTimeValid) {
        uint8_t h, m, s;
        getGpsTime(&h, &m, &s);
        int offset = getUtcOffsetFromLongitude(currentLon);
        uint8_t localH = (h + offset + 24) % 24;
        Serial.printf("[GPS] Time: %02d:%02d:%02d UTC (Local: %02d:%02d:%02d UTC%+d)\n", 
                      h, m, s, localH, m, s, offset);
      }
    } else {
      unsigned long elapsed = (millis() - gpsStartTime) / 1000;
      Serial.printf("[GPS] Acquiring fix... (%lu sec elapsed)\n", elapsed);
      Serial.printf("[GPS] Satellites visible: %d\n", currentSats);
      
      if (gpsTimeValid) {
        Serial.printf("[GPS] Time available: %02d:%02d:%02d UTC\n", 
                      gps.time.hour(), gps.time.minute(), gps.time.second());
      }
    }
    
    Serial.printf("[GPS] NMEA Stats:\n");
    Serial.printf("[GPS]   Chars processed: %lu\n", gps.charsProcessed());
    Serial.printf("[GPS]   Valid sentences: %lu\n", gps.passedChecksum());
    Serial.printf("[GPS]   Failed checksum: %lu\n", gps.failedChecksum());
    
    // Analisis hasil
    if (gps.charsProcessed() > 0 && gps.passedChecksum() == 0) {
      Serial.println(F("[GPS] WARNING: Receiving data but no valid NMEA sentences!"));
      Serial.println(F("[GPS] This usually means:"));
      Serial.println(F("[GPS]   1. GPS antenna not connected"));
      Serial.println(F("[GPS]   2. GPS module not getting clear sky view"));
      Serial.println(F("[GPS]   3. Indoors or surrounded by buildings"));
    }
    
    Serial.println(F("[GPS] --------------------------"));
    Serial.println(F(""));
  }
  
  // Warning jika tidak ada data sama sekali
  if (gps.charsProcessed() < 10 && millis() - gpsStartTime > 5000) {
    if (millis() - lastDataWarning >= 5000) {
      lastDataWarning = millis();
      Serial.println(F(""));
      Serial.println(F("[GPS] CRITICAL: No data from GPS module!"));
      Serial.println(F("[GPS] Troubleshooting steps:"));
      Serial.println(F("[GPS]   1. Check VEXT_CTRL is HIGH"));
      Serial.println(F("[GPS]   2. Verify TX/RX wiring (RX=33, TX=34)"));
      Serial.println(F("[GPS]   3. Check GPS module power LED"));
      Serial.println(F("[GPS]   4. Try swapping TX/RX if still no data"));
      Serial.println(F(""));
    }
  }
}

uint16_t readBatteryVoltage() {
  uint16_t adc = analogRead(BATTERY_ADC_PIN);
  return (uint16_t)((adc * BATTERY_VREF * BATTERY_ADC_SCALE) / 4095);
}

bool isGpsValid() {
  return gpsValid;
}

bool isGpsTimeValid() {
  return gpsTimeValid;
}

uint8_t getGpsSatellites() {
  return currentSats;
}

unsigned long getGpsAge() {
  if (!gpsValid) return 0xFFFFFFFF;
  return millis() - lastGpsUpdate;
}

void getGpsPosition(float* lat, float* lon, float* alt) {
  if (lat) *lat = currentLat;
  if (lon) *lon = currentLon;
  if (alt) *alt = currentAlt;
}

void getGpsTime(uint8_t* hour, uint8_t* minute, uint8_t* second) {
  if (hour) *hour = gps.time.hour();
  if (minute) *minute = gps.time.minute();
  if (second) *second = gps.time.second();
}

int getUtcOffsetFromLongitude(float lon) {
  // Hitung UTC offset berdasarkan longitude
  // Setiap 15 derajat = 1 jam zona waktu
  int offset = (int)round(lon / 15.0);
  
  // Batasi offset antara -12 sampai +14
  if (offset < -12) offset = -12;
  if (offset > 14) offset = 14;
  
  return offset;
}