/*
 * Laser Tag Helm - IR + Anti-Cheating + ESP-NOW
 * Semua dalam satu file agar kompatibel dengan Arduino IDE
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

// === ESP-NOW Data ===
typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

ir_data_t lastPacket;
bool dataPending = false;
volatile bool sendOK = false;

// === Anti-Cheating ===
bool antiCheatActive = true;
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 100; // ms

// === ESP-NOW Callback ===
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  sendOK = (status == ESP_NOW_SEND_SUCCESS);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Setup anti-cheat
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(PHOTODIODE_PIN, INPUT);
  digitalWrite(IR_LED_PIN, HIGH); // Nyalakan IR LED terus-menerus

  // Setup IR receiver
  IrReceiver.begin(IR_RECEIVE_PIN, false);

  // Setup ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, HELTEC_MAC, 6);
  peer.channel = 1;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("IR Receiver Ready");
  Serial.println("Anti-Cheating System Active");
  Serial.println("ESP-NOW initialized — data dikirim ke Heltec");
}

void loop() {
  // === Cek anti-cheating setiap 100ms ===
  if (millis() - lastCheckTime >= CHECK_INTERVAL) {
    lastCheckTime = millis();
    int status = digitalRead(PHOTODIODE_PIN); // LOW = cheating
    if (status == LOW) {
      if (antiCheatActive) {
        antiCheatActive = false;
        Serial.println("  WARNING: CHEATING DETECTED!");
        Serial.println("  Sinyal IR tidak akan diproses");
      }
    } else {
      if (!antiCheatActive) {
        antiCheatActive = true;
        Serial.println("✓ Anti-Cheat System Normal - Signals Accepted");
      }
    }
  }

  // === Terima sinyal IR ===
  if (IrReceiver.decode()) {
    if (antiCheatActive) {
      Serial.println("IR signal received!");

      // Format: Protocol=NEC Address=0x4, Command=0x4, Raw-Data=0xFB04FB04, ...
      Serial.printf("Protocol=NEC Address=0x%X, Command=0x%X, Raw-Data=0x%08X, %d bits, LSB first, Gap=%luus, Duration=%luus\n",
        IrReceiver.decodedIRData.address,
        IrReceiver.decodedIRData.command,
        (uint32_t)IrReceiver.decodedIRData.decodedRawData,
        IrReceiver.decodedIRData.numberOfBits,
        IrReceiver.irparams.initialGapTicks * MICROS_PER_TICK,
        IrReceiver.decodedIRData.rawlen * MICROS_PER_TICK
      );

      Serial.printf("Received NEC Signal | Address: 0x%X | Command: 0x%X\n",
        IrReceiver.decodedIRData.address,
        IrReceiver.decodedIRData.command
      );

      Serial.printf("Send with: IrSender.sendNEC(0x%X, 0x%X, <numberOfRepeats>);\n",
        IrReceiver.decodedIRData.address,
        IrReceiver.decodedIRData.command
      );

      // Kirim via ESP-NOW
      lastPacket.address = IrReceiver.decodedIRData.address;
      lastPacket.command = IrReceiver.decodedIRData.command;
      dataPending = true;
      sendOK = false;
      esp_err_t result = esp_now_send(HELTEC_MAC, (uint8_t*)&lastPacket, sizeof(lastPacket));
      if (result == ESP_OK) delay(10);

    } else {
      Serial.println("⚠️ SIGNAL REJECTED - Anti-Cheat Triggered!");
    }

    IrReceiver.resume();
  }

  // === Retry jika gagal ===
  if (dataPending && !sendOK) {
    esp_err_t result = esp_now_send(HELTEC_MAC, (uint8_t*)&lastPacket, sizeof(lastPacket));
    if (result == ESP_OK) {
      delay(10);
      if (sendOK) dataPending = false;
    }
  }

  delay(2);
}