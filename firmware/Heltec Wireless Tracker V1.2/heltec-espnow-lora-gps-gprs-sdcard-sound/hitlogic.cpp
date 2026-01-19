#include "config.h"
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

SoftwareSerial dfSerial(DFPLAYER_RX, DFPLAYER_TX);
DFRobotDFPlayerMini dfPlayer;

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  return GPS.distanceBetween(lat1, lon1, lat2, lon2); // Gunakan fungsi bawaan TinyGPS++
}

void playHitSound() {
  dfSerial.begin(9600);
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("[DFPlayer] Init failed");
    return;
  }
  dfPlayer.volume(20);
  dfPlayer.play(1); // Main 0001.mp3
  delay(1000);
}

void sendHitData() {
  // Sudah ditangani di loraTask via flag `hitDetected`
}