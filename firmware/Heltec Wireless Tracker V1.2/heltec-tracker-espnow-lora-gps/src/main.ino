/*
  Heltec Tracker - GPS + LoRa + ESP-NOW Firmware
  Board: Heltec Tracker V1.2 (ESP32-S3)
*/

#include "PinConfig.h"

// === ESP-NOW Variables ===
uint8_t senderMac[] = {0x10, 0x20, 0xBA, 0x65, 0x47, 0x34}; // Ganti dengan MAC pengirim Anda
ir_data_t latestIr = {0, 0};
bool newIrData = false;
unsigned long lastIrUpdate = 0;

// === ESP-NOW Callback Function ===
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

// === ESP-NOW Initialization ===
void espNowInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

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

// === Entry Point ===
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n=== Heltec Tracker: GPS + LoRa + ESP-NOW ==="));

  // Enable VEXT
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(100);

  // Init TFT
  tftInit();
  tftShowStatus("Initializing...", "Please wait");

  // Init GPS
  pinMode(GNSS_RST, OUTPUT);
  digitalWrite(GNSS_RST, HIGH);
  GPS_SERIAL.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println(F("[GPS] Initialized"));

  // Init LoRa
  if (!loraInit()) {
    tftShowStatus("ERROR", "LoRa Failed", "Check wiring");
    while (1) {
      digitalWrite(LORA_LED_PIN, HIGH);
      delay(200);
      digitalWrite(LORA_LED_PIN, LOW);
      delay(200);
    }
  }

  // Init ESP-NOW
  espNowInit();
  Serial.println(F("[System] Ready\n"));
  tftShowStatus("Ready!", "Waiting data...");
}

void loop() {
  static unsigned long lastTx = 0;
  static unsigned long backoff = 0;
  static unsigned long lastDisplay = 0;

  gpsFeed();

  if (millis() - lastDisplay > 1000) {
    tftUpdateData(latestIr.address, latestIr.command, currentLat, currentLon, currentSats);
    lastDisplay = millis();
  }

  // Kirim berkala
  if (millis() - lastTx >= LORA_TX_INTERVAL_MS + backoff) {
    lastTx = millis();
    backoff = random(BACKOFF_MIN_MS, BACKOFF_MAX_MS);
    loraSendData();

    // Reset IR lama (>5 detik)
    if (millis() - lastIrUpdate > 5000) {
      latestIr.address = 0;
      latestIr.command = 0;
      newIrData = false;
    }
  }

  // Kirim segera saat IR baru datang
  if (newIrData && (millis() - lastIrUpdate < 100)) {
    loraSendData();
    newIrData = false;
  }

  delay(10);
}