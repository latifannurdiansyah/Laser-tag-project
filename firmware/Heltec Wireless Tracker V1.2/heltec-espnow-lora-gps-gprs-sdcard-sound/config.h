// config.h
// Configuration for Heltec Tracker V1.2 Full System with ESP-NOW, SD, DFPlayer, and optional GSM

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <esp_now.h>
#include "HT_TinyGPS.h"

// === Pin Definitions ===
#define Vext_Ctrl 3
#define LED_K 21
#define LED_BUILTIN 18

// Radio (SX1262)
#define RADIO_SCLK_PIN 9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10
#define RADIO_CS_PIN 8
#define RADIO_RST_PIN 12
#define RADIO_DIO1_PIN 14
#define RADIO_BUSY_PIN 13

// GPS
#define GPS_TX_PIN 34
#define GPS_RX_PIN 33

// SD Card
#define SD_CS 4

// TFT
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42

// DFPlayer Mini (SoftwareSerial)
#define DFPLAYER_RX 25
#define DFPLAYER_TX 26

// Optional: SIM800L (UART2)
#define GSM_RX 16
#define GSM_TX 17

// Timezone
const int8_t TIMEZONE_OFFSET_HOURS = 7;

// Display
#define TFT_WIDTH 160
#define TFT_HEIGHT 80
#define TFT_MAX_PAGES 2
#define TFT_ROWS_PER_PAGE 6
#define PAGE_SWITCH_MS 3000
const int TFT_LEFT_MARGIN = 3;
const int TFT_TOP_MARGIN = 2;
const int TFT_LINE_HEIGHT = 10;

// Timing
const uint32_t UPLINK_INTERVAL_MS = 10000;
const uint8_t MAX_JOIN_ATTEMPTS = 10;
const uint32_t JOIN_RETRY_DELAY_MS = 8000;
const uint32_t SD_WRITE_INTERVAL_MS = 2000;
const size_t LOG_QUEUE_SIZE = 20;
const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(100);

// Hit logic
const float TARGET_LAT = -7.966667f; // Base station lat
const float TARGET_LON = 112.633333f; // Base station lon
const float MAX_HIT_DISTANCE = 50.0f; // 50 meter

// LoRaWAN Region
const LoRaWANBand_t Region = AS923;
const uint8_t subBand = 0;

// TTN Credentials (GANTI!)
const uint64_t joinEUI = 0x0000000000000000;
const uint64_t devEUI = 0x70B3D57ED007368B;
const uint8_t appKey[] = {
  0x6F, 0x90, 0xBD, 0x2B, 0x94, 0xD9, 0x50, 0x8B,
  0xB4, 0x69, 0x14, 0x81, 0xED, 0xF0, 0xC7, 0x6C
};
const uint8_t nwkKey[] = {
  0xD3, 0xC8, 0xC0, 0xBE, 0x77, 0x5F, 0x4C, 0x7E,
  0x8A, 0xA2, 0x19, 0x89, 0x9F, 0xF8, 0x86, 0xB8
};

// Data Structures
struct GpsData {
  bool valid;
  float lat, lng, alt;
  uint8_t satellites;
  uint8_t hour, minute, second;
};

struct LoraStatus {
  String lastEvent;
  int16_t rssi;
  float snr;
  bool joined;
};

struct TftPageData {
  String rows[TFT_ROWS_PER_PAGE];
};

struct __attribute__((packed)) DataPayload {
  uint32_t address_id;
  uint8_t sub_address_id;
  uint32_t shooter_address_id;
  uint8_t shooter_sub_address_id;
  uint8_t status;
  float lat, lng, alt;
};

// ESP-NOW
typedef struct {
  uint16_t address;
  uint8_t command;
  uint8_t seq;
} ir_data_t;

extern uint8_t senderMac[];
extern ir_data_t latestIr;
extern unsigned long irTimestamp;
extern uint8_t lastSeq;
extern uint32_t lastSeqTime;
extern bool hitDetected;

// Global Objects
extern TinyGPSPlus GPS;
extern SX1262 radio;
extern LoRaWANNode node;
extern Adafruit_ST7735 tft;
extern GFXcanvas16 framebuffer;
extern GpsData g_gpsData;
extern LoraStatus g_loraStatus;
extern TftPageData g_TftPageData[TFT_MAX_PAGES];

// FreeRTOS
extern SemaphoreHandle_t xGpsMutex;
extern SemaphoreHandle_t xLoraMutex;
extern SemaphoreHandle_t xTftMutex;
extern QueueHandle_t xLogQueue;

// Function Prototypes
void gpsTask(void *pv);
void loraTask(void *pv);
void tftTask(void *pv);
void sdCardTask(void *pv);
void espNowInit();
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void logToSd(const String &msg);
String formatLogLine();
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
void playHitSound();
void sendHitData();

#endif