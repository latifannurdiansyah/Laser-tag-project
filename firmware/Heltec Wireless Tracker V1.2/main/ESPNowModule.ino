// ESPNowModule.ino
#include "PinConfig.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>  // ⬅️ WAJIB untuk esp_wifi_set_channel

// === Definisi Variabel Global (HANYA DI SINI) ===
uint8_t senderMac[] = {0x10, 0x20, 0xBA, 0x65, 0x47, 0x34}; // Ganti sesuai kebutuhan
ir_data_t latestIr = {0, 0};
bool newIrData = false;
unsigned long lastIrUpdate = 0;

// === Callback Function ===
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(ir_data_t)) {
    ir_data_t pkt;
    memcpy(&pkt, data, len);
    Serial.printf("ESP-NOW RX: Addr=0x%04X, Cmd=0x%02X\n", pkt.address, pkt.command);
    latestIr = pkt;
    newIrData = true;
    lastIrUpdate = millis();
  }
}

// === Initialization Function ===
void espNowInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // ✅ Sekarang dikenali

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[ESP-NOW] Init failed"));
    while (1) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, senderMac, 6);
  peer.channel = 1;
  peer.encrypt = false;
  esp_now_del_peer(peer.peer_addr);
  esp_now_add_peer(&peer);

  Serial.println(F("[ESP-NOW] Initialized"));
}