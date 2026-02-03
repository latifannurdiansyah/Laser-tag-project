#include <TinyGPS++.h>

#define Vext_Ctrl 3    // Power control untuk GPS/TFT
#define GPS_RX 33      // RX ESP32 ← TX GPS module
#define GPS_TX 34      // TX ESP32 → RX GPS (tidak dipakai)

TinyGPSPlus gps;

void setup() {
  // Aktifkan power ke GPS & TFT
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  delay(100);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);  // Baudrate 115200!

  Serial.println("GPS Heltec Tracker v1.2");
  Serial.println("lat,lon,sats,alt");
}

void loop() {
  // Baca semua data dari GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Tampilkan tiap 1 detik
  if (millis() % 1000 < 10) {
    if (gps.location.isValid()) {
      Serial.print(gps.location.lat(), 6);
      Serial.print(",");
      Serial.print(gps.location.lng(), 6);
      Serial.print(",");
      Serial.print(gps.satellites.value());
      Serial.print(",");
      Serial.println(gps.altitude.meters(), 1);
    } else {
      Serial.println("NO FIX");
    }
  }
}