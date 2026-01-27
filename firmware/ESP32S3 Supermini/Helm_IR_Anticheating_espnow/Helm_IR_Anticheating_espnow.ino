/*
 * Laser Tag Receiver - Modular ESP32-S3
 * IR + Anti-Cheating + ESP-NOW
 */

#include "IRHandler.hpp"
#include "AntiCheat.hpp"
#include "ESPNowHandler.hpp"

// === Pin Configuration ===
#define IR_RECEIVE_PIN   4
#define IR_LED_PIN       6
#define PHOTODIODE_PIN   5

// === Ganti dengan MAC Heltec Tracker Anda ===
uint8_t HELTEC_MAC[] = {0x10, 0x20, 0xBA, 0x65, 0x4D, 0x14};

void setup() {
  Serial.begin(115200);
  delay(500); // Stabilisasi serial

  IRHandler::begin(IR_RECEIVE_PIN);
  AntiCheat::begin(IR_LED_PIN, PHOTODIODE_PIN);
  ESPNowHandler::begin(HELTEC_MAC);

  Serial.println("IR Receiver Ready");
  Serial.println("Anti-Cheating System Active");
  Serial.println("ESP-NOW initialized — data dikirim ke Heltec");
}

void loop() {
  // Update anti-cheat setiap ~100ms
  AntiCheat::update();

  // Cek sinyal IR
  if (IRHandler::available()) {
    if (AntiCheat::isActive()) {
      // Format output sesuai permintaan
      Serial.println("IR signal received!");

      // Baris lengkap seperti contoh
      Serial.printf("Protocol=NEC Address=0x%X, Command=0x%X, Raw-Data=0x%08X, %d bits, LSB first, Gap=%luus, Duration=%luus\n",
        IRHandler::getAddress(),
        IRHandler::getCommand(),
        (uint32_t)IrReceiver.decodedIRData.decodedRawData,
        IRHandler::getNumberOfBits(),
        IrReceiver.irparams.initialGapTicks * MICROS_PER_TICK,
        IrReceiver.decodedIRData.rawlen * MICROS_PER_TICK
      );

      // Ringkasan
      Serial.printf("Received NEC Signal | Address: 0x%X | Command: 0x%X\n",
        IRHandler::getAddress(),
        IRHandler::getCommand()
      );

      // Baris pengiriman
      Serial.printf("Send with: IrSender.sendNEC(0x%X, 0x%X, <numberOfRepeats>);\n",
        IRHandler::getAddress(),
        IRHandler::getCommand()
      );

      // Kirim via ESP-NOW
      ESPNowHandler::sendPacket(IRHandler::getAddress(), IRHandler::getCommand());

    } else {
      Serial.println("⚠️ SIGNAL REJECTED - Anti-Cheat Triggered!");
    }

    IRHandler::resume();
  }

  // Retry jika pengiriman gagal
  ESPNowHandler::retryIfNeeded();

  delay(2);
}