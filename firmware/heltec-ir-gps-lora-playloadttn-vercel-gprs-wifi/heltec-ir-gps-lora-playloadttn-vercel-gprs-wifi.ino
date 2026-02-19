// ============================================
// HELTEC TRACKER V1.1 - ALL-IN-ONE PROGRAM
// GPS + LoRaWAN + WiFi + GPRS + IR Receiver + SD Card + TFT Display
// Combined firmware with selectable transmission modes
// ============================================

// ============================================
// KONFIGURASI MODE - UBAH DI SINI SEBELUM UPLOAD
// ============================================
// Gunakan 1 = ENABLE, 0 = DISABLE
#define ENABLE_WIFI    1   // Kirim via WiFi HTTP
#define ENABLE_GPRS    0   // Kirim via GPRS (SIM900A)
#define ENABLE_LORAWAN 1   // Kirim via LoRaWAN (TTN)
#define ENABLE_SD      1   // Simpan ke SD Card

// ============================================
// LIBRARY INCLUDES - Harus di atas semua type definitions
// ============================================
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <IRremote.hpp>

#if ENABLE_WIFI
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#if ENABLE_GPRS
#define TINY_GSM_MODEM_SIM900
#define TINY_GSM_RX_BUFFER 1024
#include <HardwareSerial.h>
#include <TinyGsmClient.h>
#endif

// ============================================
// KONFIGURASI WIFI
// ============================================
// UBAH URL INI SESUAI DEPLOYMENT ANDA!
// Contoh: "https://your-app.vercel.app/api/track"
#define WIFI_SSID       "UserAndroid"
#define WIFI_PASSWORD   "55555550"
#define WIFI_API_URL    "https://mygps-tracker.vercel.app/api/track"

// ============================================
// KONFIGURASI GPRS (Indosat)
// ============================================
#define GPRS_APN       "indosatgprs"
#define GPRS_USER      "indosat"
#define GPRS_PASS      "indosat"
#define SIM_PIN        ""

// ThingSpeak Configuration
#define THINGSPEAK_URL    "api.thingspeak.com"
#define THINGSPEAK_PORT   80
#define THINGSPEAK_API_KEY "N6ATMD4JVI90HTHH"

// ============================================
// KONFIGURASI DEVICE
// ============================================
#define DEVICE_ID       "Heltec-P1"
#define IR_RECEIVE_PIN  5

// ============================================
// LoRaWAN Credentials - GANTI DENGAN MILIK ANDA DARI TTN!
// ============================================
const uint64_t joinEUI = 0x0000000000000003;
const uint64_t devEUI  = 0x70B3D57ED0075AC0;

const uint8_t appKey[] = {
    0x48, 0xF0, 0x3F, 0xDD, 0x9C, 0x5A, 0x9E, 0xBA,
    0x42, 0x83, 0x01, 0x1D, 0x7D, 0xBB, 0xF3, 0xF8
};

const uint8_t nwkKey[] = {
    0x9B, 0xF6, 0x12, 0xF4, 0xAA, 0x33, 0xDB, 0x73,
    0xAA, 0x1B, 0x7A, 0xC6, 0x4D, 0x70, 0x02, 0x26
};

// ============================================
// PIN DEFINITIONS
// ============================================
#define Vext_Ctrl      3
#define LED_K          21

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

// GPRS (SIM900A)
#define GPRS_TX_PIN 17
#define GPRS_RX_PIN 16
#define GPRS_RST_PIN 15

// SD Card
#define SD_CS 4

// TFT
#define TFT_CS   38
#define TFT_DC   40
#define TFT_RST  39
#define TFT_SCLK 41
#define TFT_MOSI 42

// ============================================
// TIMING CONSTANTS
// ============================================
const int8_t TIMEZONE_OFFSET_HOURS = 7;
const uint32_t UPLINK_INTERVAL_MS = 1000;
const uint32_t GPRS_INTERVAL_MS = 5000;
const uint8_t  MAX_JOIN_ATTEMPTS = 10;
const uint32_t JOIN_RETRY_DELAY_MS = 1000;
const uint32_t SD_WRITE_INTERVAL_MS = 2000;
const size_t   LOG_QUEUE_SIZE = 20;
const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(100);

const LoRaWANBand_t Region = AS923;
const uint8_t subBand = 0;

// ============================================
// DISPLAY CONSTANTS
// ============================================
#define TFT_WIDTH        160
#define TFT_HEIGHT       80
#define TFT_MAX_PAGES    3    // LoRa, GPS, IR (simplified like working firmware)
#define TFT_ROWS_PER_PAGE 6
#define PAGE_SWITCH_MS   3000

const int TFT_LEFT_MARGIN  = 3;
const int TFT_TOP_MARGIN   = 2;
const int TFT_LINE_HEIGHT  = 10;

// ============================================
// DATA STRUCTURES
// ============================================
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

struct WiFiStatus {
    bool connected;
    String ip;
    unsigned long lastReconnect;
};

struct GprsStatus {
    String lastEvent;
    bool connected;
    String ip;
    uint32_t lastAttempt;
};

struct IRStatus {
    bool dataReceived;
    uint16_t address;
    uint8_t command;
    String protocol;
    unsigned long lastTime;
};

struct TftPageData {
    String rows[TFT_ROWS_PER_PAGE];
};

// LoRaWAN Payload Structure (32-byte aligned)
struct __attribute__((packed)) DataPayload {
    uint32_t address_id;
    uint8_t  sub_address_id;
    uint32_t shooter_address_id;
    uint8_t  shooter_sub_address_id;
    uint8_t  status;
    float    lat;
    float    lng;
    float    alt;
    uint16_t battery_mv;
    uint8_t  satellites;
    int16_t  rssi;
    int8_t   snr;
};

// ============================================
// GLOBAL OBJECTS
// ============================================
TinyGPSPlus GPS;
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(TFT_WIDTH, TFT_HEIGHT);

#if ENABLE_GPRS
HardwareSerial sim900ASerial(2);
TinyGsm modem(sim900ASerial);
TinyGsmClient gprsClient(modem);
#endif

// Global shared data
GpsData g_gpsData = {
    .valid = false,
    .lat = 0.0f,
    .lng = 0.0f,
    .alt = 0.0f,
    .satellites = 0,
    .hour = 0,
    .minute = 0,
    .second = 0
};

LoraStatus g_loraStatus = {
    .lastEvent = "IDLE",
    .rssi = 0,
    .snr = 0.0f,
    .joined = false
};

WiFiStatus g_wifiStatus = {
    .connected = false,
    .ip = "N/A",
    .lastReconnect = 0
};

GprsStatus g_gprsStatus = {
    .lastEvent = "IDLE",
    .connected = false,
    .ip = "N/A",
    .lastAttempt = 0
};

IRStatus g_irStatus = {
    .dataReceived = false,
    .address = 0,
    .command = 0,
    .protocol = "NONE",
    .lastTime = 0
};

TftPageData g_TftPageData[TFT_MAX_PAGES] = {
    {"LoRaWAN Status", "-", "-", "-", "-", "-"},
    {"GPS Status", "Lat: -", "Long: -", "Alt: -", "Sat: -", "Time: --:--:--"},
    {"IR Receiver", "Waiting...", "-", "-", "-", "-"}
};

// ============================================
// FreeRTOS HANDLES
// ============================================
SemaphoreHandle_t xGpsMutex = NULL;
SemaphoreHandle_t xLoraMutex = NULL;
SemaphoreHandle_t xTftMutex = NULL;
SemaphoreHandle_t xWifiMutex = NULL;
SemaphoreHandle_t xGprsMutex = NULL;
SemaphoreHandle_t xIrMutex = NULL;
QueueHandle_t xLogQueue = NULL;

TaskHandle_t xGpsTaskHandle = NULL;
TaskHandle_t xLoraTaskHandle = NULL;
TaskHandle_t xTftTaskHandle = NULL;
TaskHandle_t xSdTaskHandle = NULL;
TaskHandle_t xWifiTaskHandle = NULL;
TaskHandle_t xGprsTaskHandle = NULL;
TaskHandle_t xIrTaskHandle = NULL;

// Flags
bool sdInitialized = false;
bool g_sendImmediately = false;
bool g_irPending = false;
uint16_t g_pendingAddress = 0;
uint8_t g_pendingCommand = 0;

// ============================================
// FORWARD DECLARATIONS
// ============================================
void gpsTask(void *pv);
void loraTask(void *pv);
void tftTask(void *pv);
void sdCardTask(void *pv);
void wifiTask(void *pv);
void gprsTask(void *pv);
void irTask(void *pv);
void logToSd(const char* msg);
const char* formatLogLine();
void sendToWiFiAPI(float lat, float lng);
void sendToGPRS(float lat, float lng);
void saveToSDOffline(float lat, float lng, float alt, uint8_t satellites,
                    uint16_t battery, int16_t rssi, float snr, const char* irStatus);
bool uploadFromSD();
String stateDecode(const int16_t result);
void ensureSDInit();

// ============================================
// SETUP
// ============================================
void setup()
{
    pinMode(Vext_Ctrl, OUTPUT);
    digitalWrite(Vext_Ctrl, HIGH);
    delay(100);
    pinMode(LED_K, OUTPUT);
    digitalWrite(LED_K, HIGH);

#ifndef LED_BUILTIN
#define LED_BUILTIN 18
#endif
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

#if ENABLE_GPRS
    pinMode(GPRS_RST_PIN, OUTPUT);
    digitalWrite(GPRS_RST_PIN, HIGH);
    delay(200);
    digitalWrite(GPRS_RST_PIN, LOW);
    delay(200);
    digitalWrite(GPRS_RST_PIN, HIGH);
    delay(1000);
#endif

    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);

    Serial.println("\n\n========================================");
    Serial.println("GPS Tracker - 4-Mode Transmission");
    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);
    
    const char* wifiStatus = ENABLE_WIFI ? "ENABLED" : "DISABLED";
    const char* gprsStatus = ENABLE_GPRS ? "ENABLED" : "DISABLED";
    const char* loraStatus = ENABLE_LORAWAN ? "ENABLED" : "DISABLED";
    const char* sdStatus = ENABLE_SD ? "ENABLED" : "DISABLED";
    
    Serial.printf("WiFi: %s, GPRS: %s, LoRaWAN: %s, SD: %s\n", 
        wifiStatus, gprsStatus, loraStatus, sdStatus);
    Serial.println("========================================\n");

    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#if ENABLE_GPRS
    sim900ASerial.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
    delay(1000);
#endif

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    tft.initR(INITR_MINI160x80);
    tft.setRotation(1);
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print("GPS 4-Mode Tracker");
    tft.setCursor(5, 20);
    tft.print(DEVICE_ID);
    tft.setCursor(5, 35);
    tft.print("Initializing...");
    delay(1500);

    xGpsMutex = xSemaphoreCreateMutex();
    xLoraMutex = xSemaphoreCreateMutex();
    xTftMutex = xSemaphoreCreateMutex();
    xWifiMutex = xSemaphoreCreateMutex();
    xGprsMutex = xSemaphoreCreateMutex();
    xIrMutex = xSemaphoreCreateMutex();

    xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(String *));
    if (!xLogQueue) {
        while (1) delay(1000);
    }

    xTaskCreatePinnedToCore(gpsTask, "GPS", 16384, NULL, 2, &xGpsTaskHandle, 1);
    xTaskCreatePinnedToCore(loraTask, "LoRa", 16384, NULL, 2, &xLoraTaskHandle, 1);
    xTaskCreatePinnedToCore(tftTask, "TFT", 16384, NULL, 1, &xTftTaskHandle, 0);
    xTaskCreatePinnedToCore(sdCardTask, "SD", 16384, NULL, 1, &xSdTaskHandle, 0);
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 16384, NULL, 1, &xWifiTaskHandle, 0);
    xTaskCreatePinnedToCore(irTask, "IR", 4096, NULL, 1, &xIrTaskHandle, 0);
    
#if ENABLE_GPRS
    xTaskCreatePinnedToCore(gprsTask, "GPRS", 65536, NULL, 0, &xGprsTaskHandle, 0);
#endif
}

void loop()
{
    delay(1000);
}

// ============================================
// GPS TASK
// ============================================
void gpsTask(void *pv)
{
    for (;;)
    {
        while (Serial1.available() > 0) {
            GPS.encode(Serial1.read());
        }

        bool hasTime = GPS.time.isValid();
        bool hasLoc = GPS.location.isValid();

        if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE)
        {
            g_gpsData.valid = hasTime || hasLoc;
            if (hasLoc) {
                g_gpsData.lat = GPS.location.lat();
                g_gpsData.lng = GPS.location.lng();
                g_gpsData.alt = GPS.altitude.meters();
            } else {
                g_gpsData.lat = g_gpsData.lng = g_gpsData.alt = 0.0f;
            }
            g_gpsData.satellites = GPS.satellites.value();
            if (hasTime) {
                g_gpsData.hour = GPS.time.hour();
                g_gpsData.minute = GPS.time.minute();
                g_gpsData.second = GPS.time.second();
            } else {
                g_gpsData.hour = g_gpsData.minute = g_gpsData.second = 0;
            }
            xSemaphoreGive(xGpsMutex);
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE)
        {
            g_TftPageData[1].rows[0] = "GPS Status";
            g_TftPageData[1].rows[1] = hasLoc ? ("Lat: " + String(g_gpsData.lat, 6)) : "Lat: -";
            g_TftPageData[1].rows[2] = hasLoc ? ("Long: " + String(g_gpsData.lng, 6)) : "Long: -";
            g_TftPageData[1].rows[3] = hasLoc ? ("Alt: " + String(g_gpsData.alt, 1) + " m") : "Alt: -";
            g_TftPageData[1].rows[4] = "Sat: " + String(g_gpsData.satellites);

            int8_t displayHour = 0;
            if (hasTime) {
                displayHour = g_gpsData.hour + TIMEZONE_OFFSET_HOURS;
                if (displayHour >= 24) displayHour -= 24;
                else if (displayHour < 0) displayHour += 24;
            }

            char timeBuf[12];
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                     hasTime ? displayHour : 0,
                     hasTime ? g_gpsData.minute : 0,
                     hasTime ? g_gpsData.second : 0);
            g_TftPageData[1].rows[5] = "Time: " + String(timeBuf);
            xSemaphoreGive(xTftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#if ENABLE_LORAWAN
// ============================================
// LORAWAN TASK
// ============================================
void loraTask(void *pv)
{
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        char err[64];
        snprintf(err, sizeof(err), "Radio init failed: %s", stateDecode(state).c_str());
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        char err[64];
        snprintf(err, sizeof(err), "LoRaWAN init failed: %s", stateDecode(state).c_str());
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    uint8_t attempts = 0;
    while (attempts++ < MAX_JOIN_ATTEMPTS)
    {
        Serial.printf("[LoRa] Join: ATTEMPT %d/%d\n", attempts, MAX_JOIN_ATTEMPTS);
        char joinMsg[32];
        snprintf(joinMsg, sizeof(joinMsg), "Join attempt %d", attempts);
        logToSd(joinMsg);

        state = node.activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.println("[LoRa] Join: SUCCESS");
            logToSd("Join successful!");
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.joined = true;
                g_loraStatus.lastEvent = "JOINED";
                xSemaphoreGive(xLoraMutex);
            }
            break;
        } else {
            Serial.printf("[LoRa] Join: FAILED (%s)\n", stateDecode(state).c_str());
            delay(JOIN_RETRY_DELAY_MS);
        }
    }

    if (!g_loraStatus.joined) {
        Serial.println("[LoRa] Join: MAX ATTEMPTS - Deleting task");
        vTaskDelete(NULL);
    }

    uint32_t lastUplink = 0;
    for (;;)
    {
        if (g_sendImmediately || millis() - lastUplink >= UPLINK_INTERVAL_MS)
        {
            g_sendImmediately = false;
            DataPayload payload;
            payload.address_id = 0x00000000;
            payload.battery_mv = 0;

            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                payload.satellites = g_gpsData.satellites;
                payload.lat = g_gpsData.lat;
                payload.lng = g_gpsData.lng;
                payload.alt = g_gpsData.alt;
                xSemaphoreGive(xGpsMutex);
            } else {
                payload.satellites = 0;
                payload.lat = payload.lng = payload.alt = 0.0f;
            }

            if (g_irPending) {
                payload.sub_address_id = g_pendingCommand;
                payload.shooter_address_id = g_pendingAddress;
                payload.shooter_sub_address_id = 1;
                payload.status = 1;
                g_irPending = false;
            } else if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                payload.sub_address_id = g_irStatus.dataReceived ? g_irStatus.command : 0;
                payload.shooter_address_id = g_irStatus.dataReceived ? g_irStatus.address : 0;
                payload.shooter_sub_address_id = g_irStatus.dataReceived ? 1 : 0;
                payload.status = g_irStatus.dataReceived ? 1 : 0;
                xSemaphoreGive(xIrMutex);
            } else {
                payload.sub_address_id = 0;
                payload.shooter_address_id = 0;
                payload.shooter_sub_address_id = 0;
                payload.status = 0;
            }

            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                payload.rssi = g_loraStatus.rssi;
                payload.snr = (int8_t)(g_loraStatus.snr + 0.5f);
                xSemaphoreGive(xLoraMutex);
            } else {
                payload.rssi = 0;
                payload.snr = 0;
            }

            uint8_t buffer[sizeof(DataPayload)];
            memcpy(buffer, &payload, sizeof(DataPayload));

            state = node.sendReceive(buffer, sizeof(buffer));
            String eventStr;
            float snr = 0.0f;
            int16_t rssi = 0;

            if (state < RADIOLIB_ERR_NONE) {
                eventStr = "TX_FAIL";
                Serial.printf("[LoRa] TX: FAILED\n");
            } else {
                if (state > 0) {
                    snr = radio.getSNR();
                    rssi = radio.getRSSI();
                    eventStr = "TX+RX_OK";
                } else {
                    eventStr = "TX_OK";
                }
                Serial.printf("[LoRa] TX: SUCCESS | RSSI: %d | SNR: %.1f\n", rssi, snr);
                
                if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_irStatus.dataReceived = false;
                    xSemaphoreGive(xIrMutex);
                }
            }

            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.lastEvent = eventStr;
                g_loraStatus.snr = snr;
                g_loraStatus.rssi = rssi;
                xSemaphoreGive(xLoraMutex);
            }

            if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[0].rows[0] = "LoRaWAN Status";
                g_TftPageData[0].rows[1] = "Event: " + eventStr;
                g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
                g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
                g_TftPageData[0].rows[4] = "Batt: N/A";
                g_TftPageData[0].rows[5] = "Joined: YES";
                xSemaphoreGive(xTftMutex);
            }

            logToSd(formatLogLine());
            lastUplink = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

#if ENABLE_WIFI
// ============================================
// WIFI TASK
// ============================================
void wifiTask(void *pv)
{
    Serial.printf("[WiFi] Connecting: %s\n", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        Serial.printf("[WiFi] Connected | IP: %s\n", ip.c_str());
        if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_wifiStatus.connected = true;
            g_wifiStatus.ip = ip;
            xSemaphoreGive(xWifiMutex);
        }
        uploadFromSD();
    } else {
        Serial.println("[WiFi] Failed");
    }

    uint32_t lastAPIUpload = 0;
    uint32_t lastSDCheck = 0;
    
    for (;;)
    {
        if (WiFi.status() != WL_CONNECTED) {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_wifiStatus.connected = false;
                xSemaphoreGive(xWifiMutex);
            }
            if (millis() - g_wifiStatus.lastReconnect >= 5000) {
                g_wifiStatus.lastReconnect = millis();
                Serial.println("[WiFi] Reconnecting...");
                WiFi.reconnect();
            }
        } else {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                if (!g_wifiStatus.connected) {
                    g_wifiStatus.connected = true;
                    Serial.printf("[WiFi] Connected | IP: %s\n", WiFi.localIP().toString().c_str());
                    uploadFromSD();
                }
                xSemaphoreGive(xWifiMutex);
            }
        }

        if (g_wifiStatus.connected && millis() - lastAPIUpload >= 1000) {
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                if (g_gpsData.valid && GPS.location.isValid()) {
                    sendToWiFiAPI(g_gpsData.lat, g_gpsData.lng);
                }
                xSemaphoreGive(xGpsMutex);
            }
            lastAPIUpload = millis();
        }

        if (g_wifiStatus.connected && millis() - lastSDCheck >= 30000) {
            uploadFromSD();
            lastSDCheck = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sendToWiFiAPI(float lat, float lng)
{
    char irStatus[32] = "-";
    float alt = 0.0f;
    uint8_t satellites = 0;
    int16_t rssi = 0;
    float snr = 0.0f;
    uint16_t irAddress = 0;
    uint8_t irCommand = 0;

    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_irStatus.dataReceived) {
            snprintf(irStatus, sizeof(irStatus), "HIT: 0x%X-0x%X", g_irStatus.address, g_irStatus.command);
            irAddress = g_irStatus.address;
            irCommand = g_irStatus.command;
        }
        xSemaphoreGive(xIrMutex);
    }

    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        satellites = g_gpsData.satellites;
        alt = g_gpsData.alt;
        xSemaphoreGive(xGpsMutex);
    }

    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        rssi = g_loraStatus.rssi;
        snr = g_loraStatus.snr;
        xSemaphoreGive(xLoraMutex);
    }

    HTTPClient http;
    http.begin(WIFI_API_URL);
    http.addHeader("Content-Type", "application/json");

    char payload[350];
    snprintf(payload, sizeof(payload),
        "{\"source\":\"wifi\","
        "\"id\":\"%s\","
        "\"lat\":%.6f,"
        "\"lng\":%.6f,"
        "\"alt\":%.1f,"
        "\"irStatus\":\"%s\","
        "\"address\":%u,"
        "\"command\":%u,"
        "\"battery\":%u,"
        "\"satellites\":%u,"
        "\"rssi\":%d,"
        "\"snr\":%.1f}",
        DEVICE_ID, lat, lng, alt, irStatus, irAddress, irCommand,
        0, satellites, rssi, snr);

    int httpCode = http.POST(payload);
    http.end();
    
    Serial.printf("[WiFi] Upload: %s | Code: %d\n", 
                  (httpCode == 200 || httpCode == 201) ? "SUCCESS" : "FAILED", httpCode);

    saveToSDOffline(lat, lng, alt, satellites, 0, rssi, snr, irStatus);
}
#endif

#if ENABLE_GPRS
// ============================================
// GPRS TASK
// ============================================
void gprsTask(void *pv)
{
    Serial.println("[GPRS] Initializing modem...");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (!modem.begin()) {
        String err = "[GPRS] Init failed";
        Serial.println(err);
        logToSd(err.c_str());
        vTaskDelete(NULL);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));

    if (strlen(SIM_PIN) > 0 && !modem.simUnlock(SIM_PIN)) {
        String err = "[GPRS] SIM unlock failed";
        Serial.println(err);
        logToSd(err.c_str());
        vTaskDelete(NULL);
    }

    bool gprsConnected = false;
    for (int retry = 0; retry < 5; retry++) {
        Serial.printf("[GPRS] Connecting to APN (attempt %d/5)...\n", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
            gprsConnected = true;
            String ip = modem.getLocalIP();
            logToSd(("[GPRS] Connected, IP=" + ip).c_str());
            Serial.println("[GPRS] Connected, IP=" + ip);
            
            if (xSemaphoreTake(xGprsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_gprsStatus.connected = true;
                g_gprsStatus.ip = ip;
                g_gprsStatus.lastEvent = "CONNECTED";
                xSemaphoreGive(xGprsMutex);
            }
            break;
        }
        logToSd(("[GPRS] Connect retry " + String(retry + 1)).c_str());
        
        for (int i = 0; i < 50; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    if (!gprsConnected) {
        logToSd("[GPRS] Failed to establish connection");
        Serial.println("[GPRS] Failed to establish connection");
    }

    uint32_t lastScheduledSend = 0;
    for (;;) {
        if (!gprsConnected || !modem.isGprsConnected()) {
            logToSd("[GPRS] Connection lost - reconnecting");
            Serial.println("[GPRS] Connection lost - reconnecting");
            vTaskDelay(pdMS_TO_TICKS(100));
            
            if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
                gprsConnected = true;
                String ip = modem.getLocalIP();
                logToSd(("[GPRS] Reconnected, IP=" + ip).c_str());
                Serial.println("[GPRS] Reconnected, IP=" + ip);
                
                if (xSemaphoreTake(xGprsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_gprsStatus.connected = true;
                    g_gprsStatus.ip = ip;
                    xSemaphoreGive(xGprsMutex);
                }
            } else {
                gprsConnected = false;
                if (xSemaphoreTake(xGprsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_gprsStatus.connected = false;
                    xSemaphoreGive(xGprsMutex);
                }
                for (int i = 0; i < 50; i++) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                continue;
            }
        }

        if (millis() - lastScheduledSend >= GPRS_INTERVAL_MS) {
            float lat = 0.0f, lng = 0.0f, alt = 0.0f;
            uint8_t satellites = 0;
            
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                lat = g_gpsData.lat;
                lng = g_gpsData.lng;
                alt = g_gpsData.alt;
                satellites = g_gpsData.satellites;
                xSemaphoreGive(xGpsMutex);
            }

            if (g_gpsData.valid && lat != 0.0f && lng != 0.0f) {
                sendToGPRS(lat, lng);
            }
            
            lastScheduledSend = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void sendToGPRS(float lat, float lng)
{
    char irStatus[32] = "-";
    float alt = 0.0f;
    uint8_t satellites = 0;
    int16_t rssi = 0;
    float snr = 0.0f;

    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_irStatus.dataReceived) {
            snprintf(irStatus, sizeof(irStatus), "HIT: 0x%X-0x%X", g_irStatus.address, g_irStatus.command);
        }
        xSemaphoreGive(xIrMutex);
    }

    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        alt = g_gpsData.alt;
        satellites = g_gpsData.satellites;
        xSemaphoreGive(xGpsMutex);
    }

    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        rssi = g_loraStatus.rssi;
        snr = g_loraStatus.snr;
        xSemaphoreGive(xLoraMutex);
    }

    String url = String("/update?api_key=") + THINGSPEAK_API_KEY;
    url += "&field1=" + String(lat, 6);
    url += "&field2=" + String(lng, 6);
    url += "&field3=" + String(alt, 1);
    url += "&field4=" + String(satellites);
    url += "&field5=" + String(irStatus);
    url += "&field6=" + String(rssi);
    url += "&field7=" + String(snr, 1);

    String event = "HTTP_FAIL";
    String responseLine = "-";
    
    if (gprsClient.connect(THINGSPEAK_URL, THINGSPEAK_PORT)) {
        gprsClient.print(String("GET ") + url + " HTTP/1.1\r\n");
        gprsClient.print(String("Host: ") + THINGSPEAK_URL + "\r\n");
        gprsClient.print("Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (gprsClient.available() == 0) {
            if (millis() - timeout > 10000) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        
        if (gprsClient.available()) {
            responseLine = gprsClient.readStringUntil('\n');
            vTaskDelay(pdMS_TO_TICKS(10));
            while (gprsClient.available()) {
                gprsClient.read();
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            
            if (responseLine.indexOf("200") >= 0 || responseLine.indexOf("OK") >= 0) {
                event = "THING_OK";
            } else {
                event = "THING_FAIL";
            }
        }
        gprsClient.stop();
    } else {
        event = "CONN_FAIL";
        logToSd("[GPRS] ThingSpeak connect failed");
    }

    logToSd(("[ThingSpeak] " + event + " | " + responseLine).c_str());
    Serial.printf("[ThingSpeak] Upload: %s | Resp: %s\n", event.c_str(), responseLine.c_str());

    saveToSDOffline(lat, lng, alt, satellites, 0, rssi, snr, irStatus);
}
#endif

// ============================================
// IR RECEIVER TASK
// ============================================
void irTask(void *pv)
{
    for (;;)
    {
        if (IrReceiver.decode()) {
            if (IrReceiver.decodedIRData.protocol == NEC) {
                if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_irStatus.protocol = "NEC";
                    g_irStatus.address = IrReceiver.decodedIRData.address;
                    g_irStatus.command = IrReceiver.decodedIRData.command;
                    g_irStatus.dataReceived = true;
                    g_irStatus.lastTime = millis();
                    
                    g_irPending = true;
                    g_pendingAddress = g_irStatus.address;
                    g_pendingCommand = g_irStatus.command;
                    g_sendImmediately = true;
                    xSemaphoreGive(xIrMutex);
                }
                
                Serial.printf("IR Received | Addr: 0x%04X | Cmd: 0x%02X\n", 
                             g_irStatus.address, g_irStatus.command);
                char irLog[48];
                snprintf(irLog, sizeof(irLog), "IR: Addr=0x%04X Cmd=0x%02X", 
                        g_irStatus.address, g_irStatus.command);
                logToSd(irLog);
            }
            IrReceiver.resume();
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[2].rows[0] = "IR Receiver";
                if (g_irStatus.dataReceived) {
                    g_TftPageData[2].rows[1] = "Proto: " + g_irStatus.protocol;
                    g_TftPageData[2].rows[2] = "Addr: 0x" + String(g_irStatus.address, HEX);
                    g_TftPageData[2].rows[3] = "Cmd: 0x" + String(g_irStatus.command, HEX);
                    unsigned long ago = (millis() - g_irStatus.lastTime) / 1000;
                    g_TftPageData[2].rows[4] = "Last: " + String(ago) + "s ago";
                    g_TftPageData[2].rows[5] = "-";
                } else {
                    g_TftPageData[2].rows[1] = "Waiting...";
                    g_TftPageData[2].rows[2] = "-";
                    g_TftPageData[2].rows[3] = "-";
                    g_TftPageData[2].rows[4] = "-";
                    g_TftPageData[2].rows[5] = "-";
                }
                xSemaphoreGive(xIrMutex);
            }
            xSemaphoreGive(xTftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// TFT TASK
// ============================================
void tftTask(void *pv)
{
    uint32_t pageSwitch = millis();
    uint8_t currentPage = 0;

    for (;;)
    {
        if (millis() - pageSwitch >= PAGE_SWITCH_MS) {
            currentPage = (currentPage + 1) % TFT_MAX_PAGES;
            pageSwitch = millis();
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            framebuffer.fillScreen(ST7735_BLACK);
            framebuffer.setTextColor(ST7735_WHITE);
            framebuffer.setTextSize(1);
            
            framebuffer.setCursor(2, 2);
            
            bool wifiOk = false, loraOk = false;
#if ENABLE_WIFI
            if (xSemaphoreTake(xWifiMutex, 10) == pdTRUE) {
                wifiOk = g_wifiStatus.connected;
                xSemaphoreGive(xWifiMutex);
            }
#endif
#if ENABLE_LORAWAN
            if (xSemaphoreTake(xLoraMutex, 10) == pdTRUE) {
                loraOk = g_loraStatus.joined;
                xSemaphoreGive(xLoraMutex);
            }
#endif
            
            const char* wStr = "-";
            const char* lStr = "-";
#if ENABLE_WIFI
            if (wifiOk) wStr = "OK"; else wStr = "X";
#endif
#if ENABLE_LORAWAN
            if (loraOk) lStr = "OK"; else lStr = "X";
#endif
            framebuffer.printf("W:%s L:%s", wStr, lStr);
            
            for (int i = 0; i < TFT_ROWS_PER_PAGE; i++) {
                int y = TFT_TOP_MARGIN + (i + 1) * TFT_LINE_HEIGHT;
                if (y >= 80) break;
                framebuffer.setCursor(TFT_LEFT_MARGIN, y);
                framebuffer.print(g_TftPageData[currentPage].rows[i]);
            }
            xSemaphoreGive(xTftMutex);
            tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), TFT_WIDTH, TFT_HEIGHT);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#if ENABLE_SD
// ============================================
// SD CARD LOGGING TASK
// ============================================
void sdCardTask(void *pv)
{
    delay(500);
    
    sdInitialized = SD.begin(SD_CS, SPI, 8000000);
    if (!sdInitialized) {
        Serial.println("[SD] Failed");
        vTaskDelete(NULL);
    }

    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SD] Connected | Size: %luMB\n", cardSize);

    if (SD.exists("/tracker.log")) {
        SD.remove("/tracker.log");
    }

    File file;
    uint32_t lastWrite = 0;

    for (;;)
    {
        uint32_t now = millis();
        if (now - lastWrite >= SD_WRITE_INTERVAL_MS) {
            file = SD.open("/tracker.log", FILE_APPEND);
            if (file) {
                char *msg;
                while (xQueueReceive(xLogQueue, &msg, 0) == pdTRUE) {
                    file.println(msg);
                    delete[] msg;
                }
                file.close();
                lastWrite = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================
// HELPER FUNCTIONS
// ============================================
void logToSd(const char* msg)
{
#if ENABLE_SD
    char *copy = new char[strlen(msg) + 1];
    strcpy(copy, msg);
    if (xQueueSendToBack(xLogQueue, &copy, pdMS_TO_TICKS(10)) != pdTRUE) {
        delete[] copy;
    }
#endif
}

void ensureSDInit() {
    if (!sdInitialized) {
        sdInitialized = SD.begin(SD_CS, SPI, 8000000);
    }
}

void saveToSDOffline(float lat, float lng, float alt, uint8_t satellites,
                    uint16_t battery, int16_t rssi, float snr, const char* irStatus)
{
#if ENABLE_SD
    ensureSDInit();
    if (!sdInitialized) return;
    
    File file = SD.open("/offline_queue.csv", FILE_APPEND);
    if (!file) return;
    
    char line[200];
    snprintf(line, sizeof(line), "%lu,%.6f,%.6f,%.1f,%u,%u,%d,%.1f,%s",
             millis(), lat, lng, alt, satellites, battery, rssi, snr, irStatus);
    
    file.println(line);
    file.close();
#endif
}

bool uploadFromSD()
{
#if ENABLE_WIFI
    ensureSDInit();
    if (!sdInitialized || !SD.exists("/offline_queue.csv")) {
        return false;
    }
    
    File file = SD.open("/offline_queue.csv", FILE_READ);
    if (!file) return false;
    
    int uploadCount = 0;
    int failCount = 0;
    String tempFileContent = "";
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() < 10) continue;
        
        int commas[8];
        int commaCount = 0;
        for (int i = 0; i < line.length() && commaCount < 8; i++) {
            if (line.charAt(i) == ',') {
                commas[commaCount++] = i;
            }
        }
        
        if (commaCount < 7) continue;
        
        String irStatus = line.substring(commas[7] + 1);
        float lat = line.substring(commas[0] + 1, commas[1]).toFloat();
        float lng = line.substring(commas[1] + 1, commas[2]).toFloat();
        float alt = line.substring(commas[2] + 1, commas[3]).toFloat();
        uint8_t satellites = line.substring(commas[3] + 1, commas[4]).toInt();
        uint16_t battery = line.substring(commas[4] + 1, commas[5]).toInt();
        int16_t rssi = line.substring(commas[5] + 1, commas[6]).toInt();
        float snr = line.substring(commas[6] + 1, commas[7]).toFloat();
        
        HTTPClient http;
        http.begin(WIFI_API_URL);
        http.addHeader("Content-Type", "application/json");
        
        String payload = "{\"source\":\"wifi\","
                        "\"id\":\"" + String(DEVICE_ID) + "\","
                        "\"lat\":" + String(lat, 6) + ","
                        "\"lng\":" + String(lng, 6) + ","
                        "\"alt\":" + String(alt, 1) + ","
                        "\"irStatus\":\"" + irStatus + "\","
                        "\"battery\":" + String(battery) + ","
                        "\"satellites\":" + String(satellites) + ","
                        "\"rssi\":" + String(rssi) + ","
                        "\"snr\":" + String(snr, 1) + "}";
        
        int httpCode = http.POST(payload);
        http.end();
        
        if (httpCode == 200 || httpCode == 201) {
            uploadCount++;
        } else {
            failCount++;
            tempFileContent += line + "\n";
        }
        
        delay(100);
    }
    
    file.close();
    
    if (uploadCount > 0 || failCount > 0) {
        Serial.printf("[WiFi] Upload: %d success, %d failed\n", uploadCount, failCount);
    }
    
    if (uploadCount > 0) {
        File tempFile = SD.open("/offline_queue.csv", FILE_WRITE);
        if (tempFile) {
            tempFile.print(tempFileContent);
            tempFile.close();
            SD.remove("/offline_queue.csv");
            File renameFile = SD.open("/offline_queue.csv", FILE_WRITE);
            renameFile.print(tempFileContent);
            renameFile.close();
        }
    } else if (tempFileContent.length() == 0) {
        SD.remove("/offline_queue.csv");
    }
    
    return uploadCount > 0;
#else
    return false;
#endif
}

const char* formatLogLine()
{
    static char timeBuf[9];
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        int8_t logHour = g_gpsData.hour + TIMEZONE_OFFSET_HOURS;
        if (logHour >= 24) logHour -= 24;
        else if (logHour < 0) logHour += 24;

        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                 g_gpsData.valid ? logHour : 0,
                 g_gpsData.valid ? g_gpsData.minute : 0,
                 g_gpsData.valid ? g_gpsData.second : 0);
        xSemaphoreGive(xGpsMutex);
    } else {
        strcpy(timeBuf, "00:00:00");
    }

    static char event[32] = "IDLE";
    float snr = 0.0f;
    int16_t rssi = 0;
    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        strncpy(event, g_loraStatus.lastEvent.c_str(), sizeof(event) - 1);
        event[sizeof(event) - 1] = '\0';
        snr = g_loraStatus.snr;
        rssi = g_loraStatus.rssi;
        xSemaphoreGive(xLoraMutex);
    }

    char latBuf[20] = "-", lonBuf[20] = "-", altBuf[20] = "-";
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_gpsData.valid && GPS.location.isValid()) {
            dtostrf(g_gpsData.lat, 1, 6, latBuf);
            dtostrf(g_gpsData.lng, 1, 6, lonBuf);
            dtostrf(g_gpsData.alt, 1, 1, altBuf);
        }
        xSemaphoreGive(xGpsMutex);
    }

    static char logLine[150];
    snprintf(logLine, sizeof(logLine),
             "%s,%s,%.1f,%d,%s,%s,%s",
             timeBuf, event, snr, rssi, latBuf, lonBuf, altBuf);
    return logLine;
}

String stateDecode(const int16_t result) {
    switch (result) {
        case RADIOLIB_ERR_NONE: return "ERR_NONE";
        case RADIOLIB_ERR_CHIP_NOT_FOUND: return "ERR_CHIP_NOT_FOUND";
        case RADIOLIB_ERR_PACKET_TOO_LONG: return "ERR_PACKET_TOO_LONG";
        case RADIOLIB_ERR_RX_TIMEOUT: return "ERR_RX_TIMEOUT";
        case RADIOLIB_ERR_MIC_MISMATCH: return "ERR_MIC_MISMATCH";
        case RADIOLIB_ERR_INVALID_FREQUENCY: return "ERR_INVALID_FREQUENCY";
        case RADIOLIB_ERR_NETWORK_NOT_JOINED: return "ERR_NETWORK_NOT_JOINED";
        case RADIOLIB_ERR_DOWNLINK_MALFORMED: return "ERR_DOWNLINK_MALFORMED";
        case RADIOLIB_ERR_NO_RX_WINDOW: return "ERR_NO_RX_WINDOW";
        case RADIOLIB_ERR_UPLINK_UNAVAILABLE: return "ERR_UPLINK_UNAVAILABLE";
        case RADIOLIB_ERR_NO_JOIN_ACCEPT: return "ERR_NO_JOIN_ACCEPT";
        case RADIOLIB_LORAWAN_NEW_SESSION: return "LORAWAN_NEW_SESSION";
        case RADIOLIB_LORAWAN_SESSION_RESTORED: return "LORAWAN_SESSION_RESTORED";
        default: return "Unknown (" + String(result) + ")";
    }
}
