/*
  Heltec Tracker - GPS + LoRaWAN (TTN) + TFT + IR (ESP-NOW receiver)
  Mode: SENDER ke The Things Network (TTN)
*/

#include "PinConfig.h"

extern TinyGPSPlus gps;
extern Adafruit_ST7735 tft;
extern ir_data_t latestIr;
extern unsigned long irTimestamp;
extern String loraWanStatus;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n=== Heltec Tracker: GPS + LoRaWAN (TTN) ==="));

  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(100);

  tftInit();
  tftShowStatus("Initializing...", "Please wait");

  // === INISIALISASI GPS SESUAI PROGRAM DOSEN ===
  pinMode(GNSS_RST, OUTPUT);
  digitalWrite(GNSS_RST, HIGH);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); // <-- 115200, Serial1
  delay(500);
  Serial.println(F("[GPS] Initialized on Serial1 @ 115200"));

  espNowInit();
  Serial.println(F("[ESP-NOW] Ready for IR data"));

  if (!loraWanInit()) {
    Serial.println(F("[LoRaWAN] Initial join failed – will retry"));
  }

  Serial.println(F("[System] Running"));
}

void loop() {
  static unsigned long lastUplink = 0;
  static unsigned long lastDisplay = 0;
  static bool hasSentIr = false;

  // === BACA GPS SETIAP ITERASI LOOP ===
  gpsFeed();

  // Hapus IR setelah 7 detik
  if (latestIr.address != 0 && (millis() - irTimestamp > 7000)) {
    latestIr.address = 0;
    latestIr.command = 0;
    latestIr.seq = 0;
    hasSentIr = false;
  }

  // Kirim ke TTN segera saat IR diterima
  if (latestIr.address != 0 && !hasSentIr && loraWanStatus == "Joined") {
    loraWanSendData();
    hasSentIr = true;
    lastUplink = millis();
  }

  // Update TFT tiap 100 ms
  if (millis() - lastDisplay > 100) {
    tftUpdateData(
      latestIr.address,
      latestIr.command,
      currentLat,
      currentLon,
      currentSats,
      currentHour,
      currentMinute,
      currentSecond,
      timeValid,
      loraWanStatus
    );
    lastDisplay = millis();
  }

  // Kirim reguler tiap 10 detik
  if (loraWanStatus == "Joined" && millis() - lastUplink >= UPLINK_INTERVAL_MS) {
    ir_data_t temp = latestIr;
    latestIr = {0, 0, 0};
    loraWanSendData();
    latestIr = temp;
    lastUplink = millis();
  }

  // Coba join ulang tiap 15 detik
  if (loraWanStatus == "JoinFailed" && (millis() % 15000 < 100)) {
    loraWanInit();
  }

  delay(5); // Ringan, tidak mengganggu pembacaan GPS
}