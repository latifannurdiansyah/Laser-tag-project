#include "config.h"
#include <esp_wifi.h>

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(ir_data_t)) return;

  ir_data_t pkt;
  memcpy(&pkt, data, len);

  uint32_t now = millis();
  bool isDuplicate = (pkt.seq == lastSeq && (now - lastSeqTime) < 300);
  bool isValid = (pkt.address != 0);

  if (!isDuplicate && isValid) {
    latestIr = pkt;
    irTimestamp = now;
    lastSeq = pkt.seq;
    lastSeqTime = now;
    hitDetected = true;
    Serial.printf("ESP-NOW RX: Addr=0x%04X, Cmd=0x%02X, Seq=%d\n", pkt.address, pkt.command, pkt.seq);
  }
}

void espNowInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, senderMac, 6);
  peer.channel = 1;
  peer.encrypt = false;
  esp_now_del_peer(peer.peer_addr);
  esp_now_add_peer(&peer);

  Serial.println("[ESP-NOW] Initialized");
}