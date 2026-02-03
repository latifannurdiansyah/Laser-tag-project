/*
 * Laser Tag Helm 
 * IR + Anti-Cheating + ESP-NOW
 */

#include <Arduino.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// === Pin Configuration ===
#define IR_RECEIVE_PIN   4
#define IR_LED_PIN       6
#define PHOTODIODE_PIN   5

// === Ganti dengan MAC Heltec Tracker Anda ===
uint8_t HELTEC_MAC[] = {0x10, 0x20, 0xBA, 0x65, 0x4D, 0x14};

// === Include modul (header-only) ===
#include "IRHandler.h"
#include "AntiCheat.h"
#include "ESPNowHandler.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  IRHandler::begin(IR_RECEIVE_PIN);
  AntiCheat::begin(IR_LED_PIN, PHOTODIODE_PIN);
  ESPNowHandler::begin(HELTEC_MAC);

  Serial.println("IR Receiver Ready");
  Serial.println("Anti-Cheating System Active");
  Serial.println("ESP-NOW initialized — data dikirim ke Heltec");
}

void loop() {
  AntiCheat::update();

  if (IRHandler::available()) {
    if (AntiCheat::isActive()) {
      Serial.println("IR signal received!");

      Serial.printf("Protocol=NEC Address=0x%X, Command=0x%X, Raw-Data=0x%08X, %d bits, LSB first, Gap=%luus, Duration=%luus\n",
        IRHandler::getAddress(),
        IRHandler::getCommand(),
        IRHandler::getDecodedRawData(),
        IRHandler::getNumberOfBits(),
        IRHandler::getInitialGapMicros(),
        IRHandler::getDurationMicros()
      );

      Serial.printf("Received NEC Signal | Address: 0x%X | Command: 0x%X\n",
        IRHandler::getAddress(),
        IRHandler::getCommand()
      );

      Serial.printf("Send with: IrSender.sendNEC(0x%X, 0x%X, <numberOfRepeats>);\n",
        IRHandler::getAddress(),
        IRHandler::getCommand()
      );

      ESPNowHandler::sendPacket(IRHandler::getAddress(), IRHandler::getCommand());

    } else {
      Serial.println("⚠️ SIGNAL REJECTED - Anti-Cheat Triggered!");
    }

    IRHandler::resume();
  }

  ESPNowHandler::retryIfNeeded();
  delay(2);
}