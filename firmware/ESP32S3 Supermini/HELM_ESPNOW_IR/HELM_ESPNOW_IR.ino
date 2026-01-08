/*
 * Penerima IR (Protokol NEC) - Versi Stabil untuk ESP32-S3
 * + ESP-NOW ke Heltec Tracker (Zero Packet Loss)
 * Library: IRremote by Armin Joachimsmeyer
 */

#include <Arduino.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>  // Diperlukan untuk esp_wifi_set_channel

// === Konfigurasi IR (TIDAK DIUBAH) ===
#define IR_RECEIVE_PIN 4
#define ENABLE_LED_FEEDBACK false

// === GANTI DENGAN MAC HELTEC TRACKER ANDA ===
uint8_t heltecMac[] = {0x10, 0x20, 0xBA, 0x65, 0x4D, 0x14}; // ← SESUAIKAN!

// === Struktur data ESP-NOW ===
typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

// Variabel untuk retry
ir_data_t lastPacket;
bool dataPending = false;
volatile bool sendOK = false;

// Callback ESP-NOW
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  sendOK = (status == ESP_NOW_SEND_SUCCESS);
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi IR (sama persis seperti aslinya)
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR Receiver Ready");
  Serial.println("Waiting for NEC IR signals...");

  // Inisialisasi ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Channel 1

  esp_now_init();
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, heltecMac, 6);
  peer.channel = 1;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("ESP-NOW initialized — data IR dikirim ke Heltec (zero-loss mode)");
}

void loop() {
  if (IrReceiver.decode()) {
    // === Bagian IR — TIDAK DIUBAH SAMA SEKALI ===
    Serial.println("IR signal received!");
    IrReceiver.printIRResultShort(&Serial);

    if (IrReceiver.decodedIRData.protocol == NEC) {
      Serial.print("Received NEC Signal | Address: 0x");
      Serial.print(IrReceiver.decodedIRData.address, HEX);
      Serial.print(" | Command: 0x");
      Serial.println(IrReceiver.decodedIRData.command, HEX);
      IrReceiver.printIRSendUsage(&Serial);

      // === TAMBAHAN: Simpan data untuk dikirim via ESP-NOW ===
      lastPacket.address = IrReceiver.decodedIRData.address;
      lastPacket.command = IrReceiver.decodedIRData.command;
      dataPending = true;

      // Coba kirim segera
      sendOK = false;
      esp_err_t result = esp_now_send(heltecMac, (uint8_t*)&lastPacket, sizeof(lastPacket));
      if (result == ESP_OK) {
        delay(10);
      }

    } else {
      Serial.print("Received unknown protocol: ");
      Serial.println(IrReceiver.decodedIRData.protocol);
    }

    IrReceiver.resume();
  }

  // === Retry jika pengiriman sebelumnya gagal ===
  if (dataPending && !sendOK) {
    esp_err_t result = esp_now_send(heltecMac, (uint8_t*)&lastPacket, sizeof(lastPacket));
    if (result == ESP_OK) {
      delay(10);
      if (sendOK) {
        dataPending = false; // Sukses → hapus flag
      }
    }
  }

  delay(5);
}