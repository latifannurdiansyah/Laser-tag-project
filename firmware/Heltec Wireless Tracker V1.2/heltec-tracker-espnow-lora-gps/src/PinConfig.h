#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>          // Dibutuhkan untuk esp_now.h
#include <esp_now.h>       // Sudah include di sini
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
#define LORA_FREQUENCY 923.0
#define LORA_BANDWIDTH 125.0
#define LORA_SPREADING_FACTOR 10
#define LORA_CODING_RATE 5
#define LORA_TX_POWER 22
#define LORA_SYNC_WORD 0x12
#define LORA_PREAMBLE_LEN 8
#define LORA_CRC true

#define NODE_ID 1
#define GROUP_ID 0x01
#define LORA_TX_INTERVAL_MS 3000
#define BACKOFF_MIN_MS 200
#define BACKOFF_MAX_MS 500

// === ESP-NOW ===
extern uint8_t senderMac[];

// === Data Structures ===
typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

typedef struct __attribute__((packed)) {
  uint8_t  header;
  uint8_t  groupId;
  uint8_t  nodeId;
  int32_t  lat;
  int32_t  lng;
  int16_t  alt;
  uint8_t  satellites;
  uint16_t battery;
  uint32_t uptime;
  uint16_t irAddress;
  uint8_t  irCommand;
  uint8_t  hitStatus;
  uint8_t  checksum;
} lora_payload_t;

// === Global Objects ===
extern SX1262 radio;
extern TinyGPSPlus gps;
extern Adafruit_ST7735 tft;

// === Global Variables ===
extern ir_data_t latestIr;
extern bool newIrData;
extern unsigned long lastIrUpdate;
extern uint32_t txCount;
extern uint32_t txErrors;
extern float currentLat, currentLon, currentAlt;
extern uint8_t currentSats;
extern bool gpsValid;

// === Function Prototypes (tanpa onDataRecv!) ===
void tftInit();
void tftShowStatus(const char* line1, const char* line2 = "", const char* line3 = "");
void tftUpdateData(uint16_t irAddr, uint8_t irCmd, float lat, float lon, uint8_t sats);
bool loraInit();
void packPayload(lora_payload_t* payload);
void loraSendData();
void loraTxBlink();
void gpsFeed();
uint16_t readBatteryVoltage();
void espNowInit();

#endif