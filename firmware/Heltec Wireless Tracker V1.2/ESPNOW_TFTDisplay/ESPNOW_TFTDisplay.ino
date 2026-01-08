#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// === Pin Konfigurasi Heltec Tracker V1.2 ===
#define VEXT_CTRL   3
#define TFT_LED_K  21
#define TFT_CS     38
#define TFT_DC     40
#define TFT_RST    39
#define TFT_SCLK   41
#define TFT_MOSI   42

// === GANTI DENGAN MAC ADDRESS PENGIRIM ===
uint8_t senderMac[] = {0x10, 0x20, 0xBA, 0x65, 0x47, 0x34}; // ← SESUAIKAN!

typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

ir_data_t latestIr = {0, 0};
bool newData = false;
unsigned long lastUpdate = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void tftInit() {
  pinMode(TFT_LED_K, OUTPUT);
  digitalWrite(TFT_LED_K, HIGH);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("IR Receiver");
  tft.drawFastHLine(0, 10, 160, ST7735_CYAN);
}

void tftShowIrData(uint16_t addr, uint8_t cmd) {
  tft.fillRect(0, 14, 160, 66, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 25);

  char buffer[40];
  snprintf(buffer, sizeof(buffer), "Addr=0x%04X, Cmd=0x%02X, Repeat=0", addr, cmd);
  tft.print(buffer);
}

void tftEspNowReady() {
  tft.fillRect(0, 14, 160, 66, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE); // putih di latar hitam
  tft.setTextSize(1);
  tft.setCursor(46, 35);
  tft.print("Waiting kiriman");
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(ir_data_t)) {
    ir_data_t pkt;
    memcpy(&pkt, data, len);
    Serial.printf("Sent: NEC Addr=0x%04X, Cmd=0x%02X, Repeat=0\n", pkt.address, pkt.command);
    latestIr = pkt;
    newData = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // 🔌 Aktifkan daya eksternal (KRUSIAL untuk Heltec Tracker V1.2)
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(50);

  tftInit();

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, senderMac, 6);
  peer.channel = 1;
  peer.encrypt = false;
  esp_now_del_peer(peer.peer_addr);
  esp_now_add_peer(&peer);

  Serial.println("Receiver siap menerima data IR via ESP-NOW");
}

void loop() {
  if (newData) {
    tftShowIrData(latestIr.address, latestIr.command);
    newData = false;
    lastUpdate = millis();
  }

  if (millis() - lastUpdate > 5000 && newData == false) {
    tftEspNowReady();
  }

  delay(10);
}