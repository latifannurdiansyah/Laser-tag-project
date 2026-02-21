// config.h
// Configuration for Heltec Tracker V1.1 Full System + GPRS + IR + PPS + Local Time
// heltec_lora_sdcard_tft_gps_gprs_miles_irremote_anti_cheat

#ifndef CONFIG_H
#define CONFIG_H

// ==============================
// GPRS & IR Configuration
// ==============================

#define TINY_GSM_MODEM_SIM900
#define TINY_GSM_RX_BUFFER 1024



#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <TinyGPS++.h>               // ← Replaced HT_TinyGPS.h
#include <HardwareSerial.h>
#include <TinyGsmClient.h>
#include <IRremote.hpp>


// Number of immediate send attempts after IR hit
const uint8_t IR_IMMEDIATE_SEND_COUNT = 2;

// Anti-Cheat IR Coverage Detection
#define IR_AMBIENT_PIN        2          // Digital input from ambient IR sensor (photodiode + comparator)
#define CHEAT_DETECTION_MS    5000       // Time (ms) of continuous cover to trigger cheat alert
#define CHEAT_CHECK_INTERVAL  100        // How often to sample (ms)
const uint32_t CHEAT_ALERT_DURATION_MS = 10000; // ← e.g., 5000 for 5 seconds

extern volatile uint32_t g_cheatAlertExpiry;
extern bool g_cheatDetected;

// ==============================
// Pin Definitions
// ==============================
#define Vext_Ctrl 3
#define LED_K 21

// Radio (SX1262)
#define RADIO_SCLK_PIN  9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10
#define RADIO_CS_PIN    8
#define RADIO_RST_PIN  12
#define RADIO_DIO1_PIN 14
#define RADIO_BUSY_PIN 13

// GPS
#define GPS_TX_PIN 34
#define GPS_RX_PIN 33
#define GNSS_RSTPin 35
#define GNSS_PPS_Pin 36             // ← PPS pin added

// GPRS (SIM900A)
#define GPRS_TX_PIN 17
#define GPRS_RX_PIN 16
#define GPRS_RST_PIN 15

// IR Receiver
#define IR_RECEIVE_PIN 5
#define ENABLE_LED_FEEDBACK false

// SD Card
#define SD_CS 4
#define SD_SCK   RADIO_SCLK_PIN
#define SD_MOSI  RADIO_MOSI_PIN
#define SD_MISO  RADIO_MISO_PIN

// TFT
#define TFT_CS   38
#define TFT_DC   40
#define TFT_RST  39
#define TFT_SCLK 41
#define TFT_MOSI 42

#ifndef LED_BUILTIN
#define LED_BUILTIN 18
#endif

// ==============================
// Display Constants
// ==============================
#define TFT_WIDTH        160
#define TFT_HEIGHT       80
#define TFT_MAX_PAGES    4
#define TFT_ROWS_PER_PAGE 6
#define PAGE_SWITCH_MS   3000

const int TFT_LEFT_MARGIN  = 3;
const int TFT_TOP_MARGIN   = 2;
const int TFT_LINE_HEIGHT  = 10;

// ==============================
// Timing & Behavior
// ==============================
const uint32_t UPLINK_INTERVAL_MS = 30000;
const uint32_t GPRS_INTERVAL_MS   = 30000;
const uint8_t  MAX_JOIN_ATTEMPTS  = 10;
const uint32_t JOIN_RETRY_DELAY_MS = 8000;
const uint32_t SD_WRITE_INTERVAL_MS = 2000;
const size_t   LOG_QUEUE_SIZE = 20;
const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(100);

// GPRS Credentials
#define APN "indosatgprs"
#define GPRS_USER "indosat"
#define GPRS_PASS "indosat"
#define SIM_PIN ""

// Server
#define SERVER "new-simpera.isar.web.id"
#define SERVER_PORT 80
#define API_KEY "tPmAT5Ab3j7F9"
// Dedicated endpoint for cheat events
#define RESOURCE_CHEAT "/end_point_cheat_events.php"

// LoRaWAN Region
const LoRaWANBand_t Region = AS923;
const uint8_t subBand = 0;

// LoRaWAN Keys
const uint64_t joinEUI = 0x0000000000000000;
const uint64_t devEUI  = 0x70B3D57ED007368B;
const uint8_t appKey[] = {0x6F, 0x90, 0xBD, 0x2B, 0x94, 0xD9, 0x50, 0x8B,
                          0xB4, 0x69, 0x14, 0x81, 0xED, 0xF0, 0xC7, 0x6C};
const uint8_t nwkKey[] = {0xD3, 0xC8, 0xC0, 0xBE, 0x77, 0x5F, 0x4C, 0x7E,
                          0x8A, 0xA2, 0x19, 0x89, 0x9F, 0xF8, 0x86, 0xB8};

// ==============================
// Data Structures
// ==============================
struct GpsData {
    bool valid;
    float lat, lng, alt;
    uint8_t satellites;
    uint8_t hour, minute, second; // ← Local time stored here
    bool hasLocation;
};

struct LoraStatus {
    String lastEvent;
    int16_t rssi;
    float snr;
    bool joined;
};

struct GprsStatus {
    String lastEvent;
    bool connected;
    uint32_t lastAttempt;
};

struct IrHitData {
    uint32_t shooter_address_id;
    uint8_t shooter_sub_address_id;
    uint8_t irHitLocation;
    bool valid;
    uint32_t timestamp;
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
    uint8_t irHitLocation;
    uint8_t cheatDetected;   // ← NEW: 1 = covered/cheating, 0 = normal
};

// ==============================
// Helper: Get UTC offset from longitude (Indonesia only)
// ==============================
inline int getUtcOffsetFromLongitude(float longitude) {
    if (longitude < 115.0f) return 7;  // WIB
    if (longitude < 134.0f) return 8;  // WITA
    return 9;                          // WIT
}

// ==============================
// Global Instances & Flags
// ==============================
extern TinyGPSPlus GPS;
extern SX1262 radio;
extern LoRaWANNode node;
extern HardwareSerial sim900ASerial;
extern TinyGsm modem;
extern TinyGsmClient client;
extern Adafruit_ST7735 tft;
extern GFXcanvas16 framebuffer;

extern GpsData g_gpsData;
extern LoraStatus g_loraStatus;
extern GprsStatus g_gprsStatus;
extern IrHitData g_irHitData;
extern TftPageData g_TftPageData[TFT_MAX_PAGES];

extern volatile bool ppsFlag;
extern volatile bool initialTimeSyncDone;

extern SemaphoreHandle_t xGpsMutex;
extern SemaphoreHandle_t xLoraMutex;
extern SemaphoreHandle_t xGprsMutex;
extern SemaphoreHandle_t xIrMutex;
extern SemaphoreHandle_t xTftMutex;
extern QueueHandle_t xLogQueue;

// ==============================
// Utility Functions
// ==============================
String stateDecode(const int16_t result);
void setSystemTimeFromGPS();
void fineTuneSystemTime();

#endif // CONFIG_H