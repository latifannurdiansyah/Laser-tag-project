#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>

// === Pin Configuration ===
#define VEXT_CTRL 3
#define LED_BUILTIN 18

#define TFT_LED_K 21
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42

#define GPS_TX_PIN 34
#define GPS_RX_PIN 33
#define GNSS_RST 35
#define GPS_SERIAL Serial2

#define BATTERY_ADC_PIN 1
#define BATTERY_VREF 3300
#define BATTERY_ADC_SCALE 2.0

// === LoRa Pins (SX1262) ===
#define RADIO_CS_PIN 8
#define RADIO_DIO1_PIN 14
#define RADIO_RST_PIN 12
#define RADIO_BUSY_PIN 13
#define RADIO_SCLK_PIN 9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10
#define LORA_LED_PIN LED_BUILTIN

// === LoRa Configuration ===
#define LORA_FREQUENCY 923.0       // MHz (Asia: 923.0, EU: 868.0, US: 915.0)
#define LORA_BANDWIDTH 125.0       // kHz
#define LORA_SPREADING_FACTOR 10   // 7-12
#define LORA_CODING_RATE 5         // 5-8
#define LORA_TX_POWER 22           // dBm (max 22 untuk SX1262)
#define LORA_SYNC_WORD 0x12
#define LORA_PREAMBLE_LEN 8
#define LORA_CRC true

#define NODE_ID 1
#define GROUP_ID 0x01
#define LORA_TX_INTERVAL_MS 3000
#define BACKOFF_MIN_MS 200
#define BACKOFF_MAX_MS 500

// === ESP-NOW ===
uint8_t senderMac[] = {0x10, 0x20, 0xBA, 0x65, 0x47, 0x34};

// === Data Structures ===
typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

typedef struct __attribute__((packed)) {
  uint8_t  header;        // 0xAA
  uint8_t  groupId;       // GROUP_ID
  uint8_t  nodeId;        // NODE_ID
  int32_t  lat;           // latitude * 1000000
  int32_t  lng;           // longitude * 1000000
  int16_t  alt;           // altitude
  uint8_t  satellites;    // GPS satellites
  uint16_t battery;       // battery voltage (mV)
  uint32_t uptime;        // uptime in seconds
  uint16_t irAddress;     // IR address
  uint8_t  irCommand;     // IR command
  uint8_t  hitStatus;     // 1=HIT, 0=NO HIT
  uint8_t  checksum;      // XOR checksum
} lora_payload_t;

// === Global Objects ===
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// === Global Variables ===
ir_data_t latestIr = {0, 0};
bool newIrData = false;
unsigned long lastIrUpdate = 0;
uint32_t txCount = 0;
uint32_t txErrors = 0;

float currentLat = 0.0;
float currentLon = 0.0;
float currentAlt = 0.0;
uint8_t currentSats = 0;
bool gpsValid = false;

// === Function Prototypes ===
void tftInit();
void tftShowStatus(const char* line1, const char* line2 = "", const char* line3 = "");
void tftUpdateData(uint16_t irAddr, uint8_t irCmd, float lat, float lon, uint8_t sats);
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
bool loraInit();
void packPayload(lora_payload_t* payload);
void loraSendData();
void loraTxBlink();
void gpsFeed();
uint16_t readBatteryVoltage();

// === TFT Functions ===
void tftInit() {
  pinMode(TFT_LED_K, OUTPUT);
  digitalWrite(TFT_LED_K, HIGH);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("GPS+LoRa Tracker");
  tft.drawFastHLine(0, 10, 160, ST7735_CYAN);
}

void tftShowStatus(const char* line1, const char* line2, const char* line3) {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  
  tft.setCursor(0, 20);
  tft.print(line1);
  if (strlen(line2) > 0) {
    tft.setCursor(0, 35);
    tft.print(line2);
  }
  if (strlen(line3) > 0) {
    tft.setCursor(0, 50);
    tft.print(line3);
  }
}

void tftUpdateData(uint16_t irAddr, uint8_t irCmd, float lat, float lon, uint8_t sats) {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  
  // IR Data
  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(1);
  tft.setCursor(0, 15);
  if (irAddr > 0) {
    tft.printf("IR: 0x%04X/0x%02X", irAddr, irCmd);
  } else {
    tft.print("IR: Waiting...");
  }
  
  // GPS Data
  tft.setTextColor(sats >= 4 ? ST7735_YELLOW : ST7735_RED);
  tft.setCursor(0, 30);
  tft.printf("GPS: %d sats", sats);
  
  if (sats >= 4) {
    tft.setCursor(0, 43);
    tft.printf("%.6f", lat);
    tft.setCursor(0, 56);
    tft.printf("%.6f", lon);
  }
  
  // TX Counter
  tft.setTextColor(ST7735_CYAN);
  tft.setCursor(0, 70);
  tft.printf("TX:%u Err:%u", txCount, txErrors);
}

// === ESP-NOW Callback ===
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

// === LoRa Functions ===
bool loraInit() {
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  pinMode(LORA_LED_PIN, OUTPUT);
  digitalWrite(LORA_LED_PIN, LOW);

  Serial.print(F("[LoRa] Initializing SX1262... "));
  
  // Reset LoRa module
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(100);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(100);
  
  // Begin dengan parameter minimal terlebih dahulu
  int state = radio.begin();
  
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Failed, code: "));
    Serial.println(state);
    Serial.println(F("[LoRa] Error codes:"));
    Serial.println(F("  -2   = SPI error"));
    Serial.println(F("  -706 = Invalid bandwidth"));
    Serial.println(F("  -707 = Invalid frequency"));
    Serial.println(F("  -708 = Invalid SF"));
    return false;
  }

  Serial.println(F("OK"));
  
  // Set parameters setelah begin() berhasil
  Serial.print(F("[LoRa] Configuring... "));
  
  state = radio.setFrequency(LORA_FREQUENCY);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Freq failed: %d\n", state);
    return false;
  }
  
  state = radio.setBandwidth(LORA_BANDWIDTH);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("BW failed: %d\n", state);
    return false;
  }
  
  state = radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("SF failed: %d\n", state);
    return false;
  }
  
  state = radio.setCodingRate(LORA_CODING_RATE);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("CR failed: %d\n", state);
    return false;
  }
  
  state = radio.setSyncWord(LORA_SYNC_WORD);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Sync failed: %d\n", state);
    return false;
  }
  
  state = radio.setOutputPower(LORA_TX_POWER);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Power failed: %d\n", state);
    return false;
  }
  
  state = radio.setPreambleLength(LORA_PREAMBLE_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Preamble failed: %d\n", state);
    return false;
  }
  
  // Enable CRC
  state = radio.setCRC(LORA_CRC);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("CRC failed: %d\n", state);
    return false;
  }

  Serial.println(F("OK"));
  Serial.printf("[LoRa] Freq: %.1f MHz, SF: %d, BW: %.1f kHz, PWR: %d dBm\n", 
                LORA_FREQUENCY, LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_TX_POWER);
  
  return true;
}

void packPayload(lora_payload_t* payload) {
  payload->header = 0xAA;
  payload->groupId = GROUP_ID;
  payload->nodeId = NODE_ID;
  
  payload->lat = (int32_t)(currentLat * 1000000);
  payload->lng = (int32_t)(currentLon * 1000000);
  payload->alt = (int16_t)(currentAlt);
  payload->satellites = currentSats;
  payload->battery = readBatteryVoltage();
  payload->uptime = millis() / 1000;
  
  payload->irAddress = latestIr.address;
  payload->irCommand = latestIr.command;
  payload->hitStatus = (latestIr.address > 0) ? 1 : 0;
  
  // Calculate checksum
  uint8_t* bytes = (uint8_t*)payload;
  uint8_t sum = 0;
  for (size_t i = 0; i < sizeof(lora_payload_t) - 1; i++) {
    sum ^= bytes[i];
  }
  payload->checksum = sum;
}

void loraSendData() {
  lora_payload_t payload;
  packPayload(&payload);
  
  int state = radio.transmit((uint8_t*)&payload, sizeof(lora_payload_t));
  
  if (state == RADIOLIB_ERR_NONE) {
    txCount++;
    loraTxBlink();
    
    Serial.printf("[LoRa TX #%u] IR:0x%04X GPS:%.6f,%.6f HIT:%d\n",
                  txCount, payload.irAddress, currentLat, currentLon, payload.hitStatus);
  } else {
    txErrors++;
    Serial.printf("[LoRa TX] Error: %d\n", state);
  }
}

void loraTxBlink() {
  digitalWrite(LORA_LED_PIN, HIGH);
  delay(50);
  digitalWrite(LORA_LED_PIN, LOW);
}

// === GPS Functions ===
void gpsFeed() {
  while (GPS_SERIAL.available() > 0) {
    char c = GPS_SERIAL.read();
    gps.encode(c);
    
    if (gps.location.isUpdated()) {
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      currentAlt = gps.altitude.meters();
      currentSats = gps.satellites.value();
      gpsValid = gps.location.isValid();
    }
  }
}

uint16_t readBatteryVoltage() {
  uint16_t adc = analogRead(BATTERY_ADC_PIN);
  return (uint16_t)((adc * BATTERY_VREF * BATTERY_ADC_SCALE) / 4095);
}

// === Setup ===
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
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.setSleep(false);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[ESP-NOW] Init failed"));
    tftShowStatus("ERROR", "ESP-NOW Failed");
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

  tftShowStatus("Ready!", "Waiting data...");
  Serial.println(F("[System] Ready\n"));
}

// === Main Loop ===
void loop() {
  static unsigned long lastTx = 0;
  static unsigned long backoff = 0;
  static unsigned long lastDisplay = 0;
  
  // Feed GPS
  gpsFeed();
  
  // Update display every 1 second
  if (millis() - lastDisplay > 1000) {
    tftUpdateData(latestIr.address, latestIr.command, currentLat, currentLon, currentSats);
    lastDisplay = millis();
  }
  
  // Send LoRa data dengan backoff untuk menghindari collision
  if (millis() - lastTx >= LORA_TX_INTERVAL_MS + backoff) {
    lastTx = millis();
    backoff = random(BACKOFF_MIN_MS, BACKOFF_MAX_MS);
    
    loraSendData();
    
    // Reset IR data setelah dikirim
    if (millis() - lastIrUpdate > 5000) {
      latestIr.address = 0;
      latestIr.command = 0;
      newIrData = false;
    }
  }
  
  // Kirim segera jika ada IR data baru
  if (newIrData && (millis() - lastIrUpdate < 100)) {
    loraSendData();
    newIrData = false;
  }
  
  delay(10);
}