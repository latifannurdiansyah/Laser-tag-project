/*
  Heltec Tracker - GPS + LoRa + ESP-NOW Firmware
  Board: Heltec Tracker V1.2 (ESP32-S3)
  
  Revision: GPS module improved with better cold start handling + Time Display
*/

#include "PinConfig.h"

void setup() {
  // Inisialisasi Serial untuk debugging
  Serial.begin(115200);
  delay(200);
  
  Serial.println(F(""));
  Serial.println(F("============================================="));
  Serial.println(F("  Heltec Tracker: GPS + LoRa + ESP-NOW"));
  Serial.println(F("============================================="));
  Serial.println(F(""));
  
  // Power control - HARUS HIGH untuk menyalakan peripheral
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(100);
  Serial.println(F("[Power] VEXT enabled"));

  // Inisialisasi TFT display
  tftInit();
  tftShowStatus("Initializing...", "Please wait");
  Serial.println(F("[TFT] Display initialized"));

  // Inisialisasi GPS dengan fungsi khusus
  gpsInit();
  tftShowStatus("GPS Starting...", "Acquiring fix");

  // Inisialisasi LoRa
  if (!loraInit()) {
    Serial.println(F("[ERROR] LoRa initialization failed!"));
    tftShowStatus("ERROR", "LoRa Failed", "Check wiring");
    
    // Blink LED sebagai indikator error
    while (1) {
      digitalWrite(LORA_LED_PIN, HIGH);
      delay(200);
      digitalWrite(LORA_LED_PIN, LOW);
      delay(200);
    }
  }

  // Inisialisasi ESP-NOW untuk menerima data IR
  espNowInit();
  
  Serial.println(F(""));
  Serial.println(F("[System] All modules initialized"));
  Serial.println(F("[System] Ready to operate"));
  Serial.println(F("============================================="));
  Serial.println(F(""));
  
  tftShowStatus("System Ready", "Waiting GPS...");
}

void loop() {
  static unsigned long lastTx = 0;
  static unsigned long backoff = 0;
  static unsigned long lastDisplay = 0;
  static bool gpsFixReported = false;

  // Feed GPS data (parsing NMEA)
  gpsFeed();

  // Update display setiap 1 detik
  if (millis() - lastDisplay > 1000) {
    lastDisplay = millis();
    
    // Update TFT dengan data terbaru
    tftUpdateData(latestIr.address, latestIr.command, currentLat, currentLon, currentSats);
    
    // Report ke Serial saat pertama kali dapat GPS fix
    if (gpsValid && !gpsFixReported) {
      Serial.println(F(""));
      Serial.println(F("[Main] GPS fix acquired - starting LoRa transmission"));
      Serial.println(F(""));
      gpsFixReported = true;
      tftShowStatus("GPS Fixed!", "Transmitting...");
    }
  }

  // Transmisi LoRa berkala
  if (millis() - lastTx >= LORA_TX_INTERVAL_MS + backoff) {
    lastTx = millis();
    backoff = random(BACKOFF_MIN_MS, BACKOFF_MAX_MS);
    
    // Kirim data via LoRa
    loraSendData();
    
    // Reset data IR jika sudah lebih dari 5 detik
    if (millis() - lastIrUpdate > 5000) {
      latestIr.address = 0;
      latestIr.command = 0;
      newIrData = false;
    }
  }

  // Transmisi LoRa segera saat ada data IR baru
  if (newIrData && (millis() - lastIrUpdate < 100)) {
    Serial.println(F("[Main] New IR data detected - immediate transmission"));
    loraSendData();
    newIrData = false;
  }

  // Small delay untuk stability
  delay(10);
}