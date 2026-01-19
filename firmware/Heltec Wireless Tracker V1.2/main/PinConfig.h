#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <esp_now.h>
#include <RadioLib.h>
#include "HT_TinyGPS.h" // Versi dari dosen

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
#define GPS_SERIAL Serial1

#define BATTERY_ADC_PIN 1
#define BATTERY_VREF 3300
#define BATTERY_ADC_SCALE 2.0

// === LoRaWAN (SX1262) ===
#define RADIO_CS_PIN 8
#define RADIO_DIO1_PIN 14
#define RADIO_RST_PIN 12
#define RADIO_BUSY_PIN 13
#define RADIO_SCLK_PIN 9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10

// === ESP-NOW ===
extern uint8_t senderMac[];

// === Konfigurasi Waktu ===
const int8_t TIMEZONE_OFFSET_HOURS = 7;

// === LoRaWAN TTN ===
const uint32_t UPLINK_INTERVAL_MS = 10000;
const LoRaWANBand_t Region = AS923;
const uint8_t subBand = 0;

// GANTI DENGAN KREDENSIAL TTN ANDA!
const uint64_t joinEUI = 0x0000000000000000;
const uint64_t devEUI  = 0x70B3D57ED007368B;
const uint8_t appKey[] = {
  0x6F, 0x90, 0xBD, 0x2B, 0x94, 0xD9, 0x50, 0x8B,
  0xB4, 0x69, 0x14, 0x81, 0xED, 0xF0, 0xC7, 0x6C
};
const uint8_t nwkKey[] = {
  0xD3, 0xC8, 0xC0, 0xBE, 0x77, 0x5F, 0x4C, 0x7E,
  0x8A, 0xA2, 0x19, 0x89, 0x9F, 0xF8, 0x86, 0xB8
};

// === Data Structures ===
typedef struct {
  uint16_t address;
  uint8_t  command;
  uint8_t  seq;        // Untuk anti duplikat
} ir_data_t;

struct __attribute__((packed)) ttn_payload_t {
  uint16_t irAddress;
  uint8_t  irCommand;
  int32_t  lat;
  int32_t  lng;
  int16_t  alt;
  uint8_t  satellites;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint8_t  hitStatus;
};

// === Global Objects ===
extern SX1262 radio;
extern LoRaWANNode node;
extern TinyGPSPlus gps;
extern Adafruit_ST7735 tft;

// === Global Variables ===
extern ir_data_t latestIr;
extern unsigned long irTimestamp;
extern uint8_t lastSeq;
extern uint32_t lastSeqTime;

extern float currentLat, currentLon, currentAlt;
extern uint8_t currentSats;
extern bool gpsValid;

extern uint8_t currentHour, currentMinute, currentSecond;
extern bool timeValid;

extern String loraWanStatus;

// === Function Prototypes ===
void tftInit();
void tftShowStatus(const char* line1, const char* line2 = "", const char* line3 = "");
void tftUpdateData(uint16_t irAddr, uint8_t irCmd, float lat, float lon, uint8_t sats,
                   uint8_t hour, uint8_t minute, uint8_t second, bool validTime,
                   const String& loraStatus);
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
bool loraWanInit();
void loraWanSendData();
void gpsFeed();
uint16_t readBatteryVoltage();
void espNowInit();

#endif