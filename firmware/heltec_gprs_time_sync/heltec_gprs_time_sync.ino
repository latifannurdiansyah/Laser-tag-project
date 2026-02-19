// heltec_gprs_time_sync.ino
// Config + Main program in one file
// Target: Heltec HT-IT WSL V3 (ESP32-S3 + SX1262 + SIM900)
// Features: GPRS + TTN (LoRaWAN) + GPS + TFT Display

// =====================================================================
// SECTION 1: DEFINES & INCLUDES
// =====================================================================

#define TINY_GSM_MODEM_SIM900
#define TINY_GSM_RX_BUFFER 1024

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <TinyGsmClient.h>
#include <IRremote.hpp>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ---- Pin Definitions ----
#define Vext_Ctrl    3
#define LED_K        21

#define RADIO_SCLK_PIN   9
#define RADIO_MISO_PIN  11
#define RADIO_MOSI_PIN  10
#define RADIO_CS_PIN     8
#define RADIO_RST_PIN   12
#define RADIO_DIO1_PIN  14
#define RADIO_BUSY_PIN  13

#define GPS_TX_PIN   34
#define GPS_RX_PIN   33
#define GNSS_RSTPin  35
#define GNSS_PPS_Pin 36

#define GPRS_TX_PIN  17
#define GPRS_RX_PIN  16
#define GPRS_RST_PIN 15

#define IR_RECEIVE_PIN     5
#define ENABLE_LED_FEEDBACK false

#define SD_CS    4
#define SD_SCK   RADIO_SCLK_PIN
#define SD_MOSI  RADIO_MOSI_PIN
#define SD_MISO  RADIO_MISO_PIN

#define TFT_CS   38
#define TFT_DC   40
#define TFT_RST  39
#define TFT_SCLK 41
#define TFT_MOSI 42

#ifndef LED_BUILTIN
#define LED_BUILTIN 18
#endif

// ---- Display Constants ----
#define TFT_WIDTH         160
#define TFT_HEIGHT         80
#define TFT_MAX_PAGES       4
#define TFT_ROWS_PER_PAGE   6
#define PAGE_SWITCH_MS   3000

const int TFT_LEFT_MARGIN = 3;
const int TFT_TOP_MARGIN  = 2;
const int TFT_LINE_HEIGHT = 10;

// ---- Timing Constants ----
const uint32_t  UPLINK_INTERVAL_MS  = 30000;
const uint32_t  GPRS_INTERVAL_MS    = 30000;
const uint8_t   MAX_JOIN_ATTEMPTS   = 10;
const uint32_t  JOIN_RETRY_DELAY_MS = 8000;
const uint32_t  SD_WRITE_INTERVAL_MS = 2000;
const size_t    LOG_QUEUE_SIZE       = 20;
const TickType_t MUTEX_TIMEOUT       = pdMS_TO_TICKS(100);

// ---- GPRS Credentials ----
#define APN        "indosatgprs"
#define GPRS_USER  "indosat"
#define GPRS_PASS  "indosat"
#define SIM_PIN    ""

// ---- Server Config ----
#define SERVER       "laser-tag-project.vercel.app"
#define RESOURCE     "/api/track"
#define SERVER_PORT  80
#define DEVICE_ID    "Heltec-P1"

// ---- LoRaWAN Config ----
const LoRaWANBand_t Region  = AS923;
const uint8_t       subBand = 0;

const uint64_t joinEUI = 0x0000000000000003;
const uint64_t devEUI  = 0x70B3D57ED0075AC0;
const uint8_t appKey[] = {0x48, 0xF0, 0x3F, 0xDD, 0x9C, 0x5A, 0x9E, 0xBA,
                           0x42, 0x83, 0x01, 0x1D, 0x7D, 0xBB, 0xF3, 0xF8};
const uint8_t nwkKey[] = {0x9B, 0xF6, 0x12, 0xF4, 0xAA, 0x33, 0xDB, 0x73,
                           0xAA, 0x1B, 0x7A, 0xC6, 0x4D, 0x70, 0x02, 0x26};

// ---- UTC offset helper ----
inline int getUtcOffsetFromLongitude(float longitude) {
    if (longitude < 115.0f) return 7;
    if (longitude < 134.0f) return 8;
    return 9;
}

// =====================================================================
// SECTION 2: GLOBAL OBJECT DEFINITIONS
// =====================================================================

TinyGPSPlus  GPS;
SX1262       radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode  node(&radio, &Region, subBand);

HardwareSerial gpsSerial(1);
HardwareSerial sim900ASerial(1);
TinyGsm        modem(sim900ASerial);
TinyGsmClient  gprsClient(modem);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16     framebuffer(TFT_WIDTH, TFT_HEIGHT);

// =====================================================================
// SECTION 3: DATA STRUCTURES
// =====================================================================

struct GpsData {
    bool    valid;
    float   lat, lng, alt;
    uint8_t satellites;
    uint8_t hour, minute, second;
    bool    hasLocation;
};

struct LoraStatus {
    String  lastEvent;
    int16_t rssi;
    float   snr;
    bool    joined;
};

struct GprsStatus {
    String   lastEvent;
    bool     connected;
    uint32_t lastAttempt;
};

struct IrHitData {
    uint32_t shooter_address_id;
    uint8_t  shooter_sub_address_id;
    uint8_t  irHitLocation;
    bool     valid;
    uint32_t timestamp;
};

struct TftPageData {
    String rows[TFT_ROWS_PER_PAGE];
};

struct __attribute__((packed)) HelmetEvent {
    uint32_t shooter_address_id;
    uint8_t  shooter_sub_address_id;
    uint8_t  irHitLocation;
};

struct __attribute__((packed)) DataPayload {
    uint32_t address_id;
    uint8_t  sub_address_id;
    uint32_t shooter_address_id;
    uint8_t  shooter_sub_address_id;
    uint8_t  status;
    float    lat, lng, alt;
    uint8_t  irHitLocation;
};

// =====================================================================
// SECTION 4: GLOBAL VARIABLES
// =====================================================================

GpsData     g_gpsData      = {};
LoraStatus  g_loraStatus   = {};
GprsStatus  g_gprsStatus   = {};
IrHitData   g_irHitData    = {};
TftPageData g_TftPageData[TFT_MAX_PAGES] = {};

volatile bool ppsFlag            = false;
bool initialTimeSyncDone = false;
bool gprsConnected = false;

SemaphoreHandle_t xGpsMutex  = NULL;
SemaphoreHandle_t xLoraMutex = NULL;
SemaphoreHandle_t xGprsMutex = NULL;
SemaphoreHandle_t xIrMutex   = NULL;
SemaphoreHandle_t xTftMutex  = NULL;
QueueHandle_t     xLogQueue         = NULL;
QueueHandle_t     xHelmetEventQueue = NULL;

// =====================================================================
// SECTION 5: HELPER FUNCTIONS
// =====================================================================

String stateDecode(const int16_t result) {
    if (result == RADIOLIB_ERR_NONE)        return "OK";
    if (result == RADIOLIB_ERR_INVALID_CRC) return "CRC";
    if (result == RADIOLIB_ERR_RX_TIMEOUT)  return "RX_TO";
    if (result == RADIOLIB_ERR_RX_PACKET_LEN) return "LEN";
    return String(result);
}

// =====================================================================
// SECTION 6: GPRS FUNCTIONS
// =====================================================================

bool initGPRS() {
    Serial.println(F("\n[GPRS] Initializing..."));
    
    sim900ASerial.begin(9600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
    
    pinMode(GPRS_RST_PIN, OUTPUT);
    digitalWrite(GPRS_RST_PIN, HIGH);
    delay(100);
    digitalWrite(GPRS_RST_PIN, LOW);
    delay(2000);
    
    uint32_t start = millis();
    while (!modem.testAT() && (millis() - start < 10000)) {
        Serial.print(".");
        delay(500);
    }
    
    if (!modem.testAT()) {
        Serial.println(F("\n[GPRS] Modem not responding"));
        gprsConnected = false;
        return false;
    }
    Serial.println(F("[GPRS] Modem OK"));
    
    Serial.print(F("[NET] Waiting for network..."));
    if (!modem.waitForNetwork(60000L)) {
        Serial.println(F(" FAILED"));
        gprsConnected = false;
        return false;
    }
    Serial.println(F(" OK"));
    
    Serial.print(F("[GPRS] Connecting APN..."));
    if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
        Serial.println(F(" FAILED"));
        gprsConnected = false;
        return false;
    }
    Serial.println(F(" OK"));
    
    gprsConnected = true;
    g_gprsStatus.connected = true;
    Serial.println(F("[GPRS] Ready!"));
    return true;
}

// =====================================================================
// SECTION 7: TIME SYNC FROM GPRS
// =====================================================================

bool syncTimeFromGPRS() {
    Serial.println(F("\n[TIME SYNC] Starting GPRS time sync..."));
    
    if (!gprsConnected) {
        if (!initGPRS()) {
            return false;
        }
    }
    
    bool success = false;
    
    // Open TCP connection to worldtimeapi.org
    Serial.print(F("[HTTP] Connecting to worldtimeapi.org:80..."));
    if (!gprsClient.connect("worldtimeapi.org", 80)) {
        Serial.println(F(" FAILED"));
        return false;
    }
    Serial.println(F(" OK"));
    
    // Send HTTP GET
    gprsClient.print(F(
        "GET /api/ip HTTP/1.1\r\n"
        "Host: worldtimeapi.org\r\n"
        "Connection: close\r\n"
        "\r\n"
    ));
    
    // Read HTTP response
    unsigned long timeout = millis() + 15000;
    String header  = "";
    String body    = "";
    bool   inBody  = false;
    
    while (gprsClient.connected() && millis() < timeout) {
        if (gprsClient.available()) {
            char c = gprsClient.read();
            if (!inBody) {
                header += c;
                if (header.endsWith("\r\n\r\n")) {
                    inBody = true;
                    if (header.indexOf("200 OK") == -1) {
                        Serial.println(F("[HTTP] Non-200 response"));
                        break;
                    }
                }
            } else {
                body += c;
            }
        }
    }
    gprsClient.stop();
    
    if (body.length() == 0) {
        Serial.println(F("[HTTP] Empty response"));
        return false;
    }
    
    // Parse unixtime
    int unixStart = body.indexOf("\"unixtime\":");
    int dtStart   = body.indexOf("\"datetime\":\"");
    if (unixStart == -1 || dtStart == -1) {
        Serial.println(F("[PARSE] Missing keys"));
        return false;
    }
    
    long unixtime = atol(body.c_str() + unixStart + 11);
    if (unixtime < 1700000000L) {
        Serial.println(F("[PARSE] Invalid unixtime"));
        return false;
    }
    
    // Parse datetime string
    int    dtEnd       = body.indexOf("\"", dtStart + 12);
    String datetimeStr = body.substring(dtStart + 12, dtEnd);
    Serial.print(F("[TIME] Datetime: "));
    Serial.println(datetimeStr);
    Serial.printf("[TIME] Unix: %ld\n", unixtime);
    
    // Apply to ESP32 system clock
    struct timeval tv;
    tv.tv_sec  = unixtime;
    tv.tv_usec = 0;
    if (settimeofday(&tv, NULL) != 0) {
        Serial.println(F("[TIME] settimeofday() failed"));
        return false;
    }
    
    // Verify
    time_t    now = time(nullptr);
    struct tm *tm = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm);
    Serial.print(F("[TIME] System time set: "));
    Serial.println(buf);
    
    initialTimeSyncDone = true;
    return true;
}

// =====================================================================
// SECTION 8: GPS FUNCTIONS
// =====================================================================

void initGPS() {
    gpsSerial.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println(F("[GPS] Serial initialized"));
}

void readGPS() {
    while (gpsSerial.available() > 0) {
        GPS.encode(gpsSerial.read());
    }
    
    bool hasLocation = GPS.location.isValid();
    bool hasTime = GPS.time.isValid();
    
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_gpsData.valid = hasLocation || hasTime;
        g_gpsData.hasLocation = hasLocation;
        
        if (hasLocation) {
            g_gpsData.lat = GPS.location.lat();
            g_gpsData.lng = GPS.location.lng();
            g_gpsData.alt = GPS.altitude.meters();
        }
        g_gpsData.satellites = GPS.satellites.value();
        
        if (hasTime) {
            g_gpsData.hour = GPS.time.hour();
            g_gpsData.minute = GPS.time.minute();
            g_gpsData.second = GPS.time.second();
        }
        xSemaphoreGive(xGpsMutex);
    }
    
    // Update TFT
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[1].rows[0] = "GPS Status";
        if (hasLocation) {
            g_TftPageData[1].rows[1] = "Lat: " + String(g_gpsData.lat, 6);
            g_TftPageData[1].rows[2] = "Lng: " + String(g_gpsData.lng, 6);
            g_TftPageData[1].rows[3] = "Alt: " + String(g_gpsData.alt, 1) + "m";
        } else {
            g_TftPageData[1].rows[1] = "Lat: -";
            g_TftPageData[1].rows[2] = "Lng: -";
            g_TftPageData[1].rows[3] = "Alt: -";
        }
        g_TftPageData[1].rows[4] = "Sat: " + String(g_gpsData.satellites);
        
        if (hasTime) {
            int8_t displayHour = g_gpsData.hour + 7;
            if (displayHour >= 24) displayHour -= 24;
            char timeBuf[12];
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", 
                     displayHour, g_gpsData.minute, g_gpsData.second);
            g_TftPageData[1].rows[5] = "Time: " + String(timeBuf);
        } else {
            g_TftPageData[1].rows[5] = "Time: -";
        }
        xSemaphoreGive(xTftMutex);
    }
}

// =====================================================================
// SECTION 9: LORAWAN FUNCTIONS
// =====================================================================

bool initLoRaWAN() {
    Serial.println(F("\n[LoRa] Initializing..."));
    
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[LoRa] Radio init failed: "));
        Serial.println(stateDecode(state));
        return false;
    }
    Serial.println(F("[LoRa] Radio OK"));
    
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[LoRa] OTAA init failed: "));
        Serial.println(stateDecode(state));
        return false;
    }
    Serial.println(F("[LoRa] OTAA ready"));
    
    // Join TTN
    uint8_t attempts = 0;
    while (attempts++ < MAX_JOIN_ATTEMPTS) {
        Serial.printf("[LoRa] Join attempt %d/%d\n", attempts, MAX_JOIN_ATTEMPTS);
        
        state = node.activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.println(F("[LoRa] JOIN SUCCESS!"));
            g_loraStatus.joined = true;
            g_loraStatus.lastEvent = "JOINED";
            return true;
        }
        
        Serial.printf("[LoRa] Join failed: %s\n", stateDecode(state).c_str());
        delay(JOIN_RETRY_DELAY_MS);
    }
    
    Serial.println(F("[LoRa] Max join attempts reached"));
    return false;
}

void sendLoRaData() {
    if (!g_loraStatus.joined) {
        return;
    }
    
    DataPayload payload;
    payload.address_id = 0x00000001;
    payload.sub_address_id = 0;
    payload.shooter_address_id = 0;
    payload.shooter_sub_address_id = 0;
    payload.status = 0;
    payload.irHitLocation = 0;
    
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        payload.lat = g_gpsData.lat;
        payload.lng = g_gpsData.lng;
        payload.alt = g_gpsData.alt;
        xSemaphoreGive(xGpsMutex);
    }
    
    uint8_t buffer[sizeof(DataPayload)];
    memcpy(buffer, &payload, sizeof(DataPayload));
    
    int16_t state = node.sendReceive(buffer, sizeof(buffer));
    
    if (state < RADIOLIB_ERR_NONE) {
        g_loraStatus.lastEvent = "TX_FAIL";
        Serial.println(F("[LoRa] TX FAILED"));
    } else {
        float snr = radio.getSNR();
        int16_t rssi = radio.getRSSI();
        g_loraStatus.snr = snr;
        g_loraStatus.rssi = rssi;
        g_loraStatus.lastEvent = state > 0 ? "TX+RX_OK" : "TX_OK";
        Serial.printf("[LoRa] TX OK | RSSI: %d | SNR: %.1f\n", rssi, snr);
    }
    
    // Update TFT
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[0].rows[0] = "LoRaWAN Status";
        g_TftPageData[0].rows[1] = "Event: " + g_loraStatus.lastEvent;
        g_TftPageData[0].rows[2] = "RSSI: " + String(g_loraStatus.rssi) + " dBm";
        g_TftPageData[0].rows[3] = "SNR: " + String(g_loraStatus.snr, 1) + " dB";
        g_TftPageData[0].rows[4] = g_loraStatus.joined ? "Joined: YES" : "Joined: NO";
        g_TftPageData[0].rows[5] = "-";
        xSemaphoreGive(xTftMutex);
    }
}

// =====================================================================
// SECTION 10: DATA TRANSMISSION VIA GPRS
// =====================================================================

void sendToServerGPRS(float lat, float lng, float alt, uint8_t satellites) {
    if (!gprsConnected) {
        Serial.println(F("[GPRS] Not connected, skipping upload"));
        return;
    }
    
    HTTPClient http;
    String url = "http://" + String(SERVER) + RESOURCE;
    http.begin(gprsClient, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    
    char payload[350];
    snprintf(payload, sizeof(payload),
        "{\"source\":\"gprs\","
        "\"id\":\"%s\","
        "\"lat\":%.6f,"
        "\"lng\":%.6f,"
        "\"alt\":%.1f,"
        "\"satellites\":%u,"
        "\"rssi\":%d,"
        "\"snr\":%.1f}",
        DEVICE_ID, lat, lng, alt, satellites, g_loraStatus.rssi, g_loraStatus.snr);
    
    Serial.print(F("[GPRS] Sending: "));
    Serial.println(payload);
    
    int httpCode = http.POST(payload);
    http.end();
    
    Serial.printf("[GPRS] HTTP Response: %d\n", httpCode);
    
    // Update TFT
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (httpCode > 0) {
            g_TftPageData[2].rows[4] = "Server: OK";
        } else {
            g_TftPageData[2].rows[4] = "Server: FAIL";
        }
        xSemaphoreGive(xTftMutex);
    }
}

// =====================================================================
// SECTION 11: TFT DISPLAY
// =====================================================================

void initTFT() {
    tft.initR(INITR_MINI160x80);
    tft.invertDisplay(false);
    tft.setRotation(1);
    framebuffer.fillScreen(ST7735_BLACK);
    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
    Serial.println(F("[TFT] Initialized"));
}

void updateTFT() {
    static uint32_t lastPageSwitch = 0;
    static uint8_t currentPage = 0;
    
    if (millis() - lastPageSwitch >= PAGE_SWITCH_MS) {
        currentPage = (currentPage + 1) % TFT_MAX_PAGES;
        lastPageSwitch = millis();
    }
    
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        framebuffer.fillScreen(ST7735_BLACK);
        framebuffer.setTextColor(ST7735_WHITE);
        framebuffer.setTextSize(1);
        
        // Status bar
        framebuffer.setCursor(2, 2);
        framebuffer.printf("G:%s L:%s", 
                          gprsConnected ? "OK" : "X",
                          g_loraStatus.joined ? "OK" : "X");
        
        // Page content
        for (int i = 0; i < TFT_ROWS_PER_PAGE; i++) {
            int y = TFT_TOP_MARGIN + (i + 1) * TFT_LINE_HEIGHT;
            if (y >= 80) break;
            framebuffer.setCursor(TFT_LEFT_MARGIN, y);
            framebuffer.print(g_TftPageData[currentPage].rows[i]);
        }
        
        xSemaphoreGive(xTftMutex);
        tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), TFT_WIDTH, TFT_HEIGHT);
    }
}

// =====================================================================
// SECTION 12: SETUP & LOOP
// =====================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    Serial.println(F("\n=== Heltec GPRS + TTN ==="));
    
    // Power on peripherals
    pinMode(Vext_Ctrl, OUTPUT);
    digitalWrite(Vext_Ctrl, HIGH);
    delay(100);
    
    pinMode(LED_K, OUTPUT);
    digitalWrite(LED_K, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Create FreeRTOS mutexes
    xGpsMutex  = xSemaphoreCreateMutex();
    xLoraMutex = xSemaphoreCreateMutex();
    xGprsMutex = xSemaphoreCreateMutex();
    xIrMutex   = xSemaphoreCreateMutex();
    xTftMutex  = xSemaphoreCreateMutex();
    
    // Initialize TFT
    initTFT();
    
    // Initialize GPS
    initGPS();
    
    // Initialize GPRS
    gprsConnected = initGPRS();
    
    // Time sync from GPRS
    if (gprsConnected) {
        syncTimeFromGPRS();
    }
    
    // Initialize LoRaWAN
    g_loraStatus.joined = initLoRaWAN();
    
    // Initialize TFT pages
    g_TftPageData[0] = {"LoRaWAN Status", "-", "-", "-", "-", "-"};
    g_TftPageData[1] = {"GPS Status", "Lat: -", "Lng: -", "Alt: -", "Sat: -", "Time: --:--:--"};
    g_TftPageData[2] = {"GPRS Status", "Connected: NO", "-", "-", "-", "-"};
    g_TftPageData[3] = {"System", "Time: -", "-", "-", "-", "-"};
    
    Serial.println(F("\n=== Setup Complete ==="));
}

void loop() {
    // Read GPS
    readGPS();
    
    // Update GPRS status
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[2].rows[0] = "GPRS Status";
        g_TftPageData[2].rows[1] = gprsConnected ? "Connected: YES" : "Connected: NO";
        xSemaphoreGive(xTftMutex);
    }
    
    // Send data periodically
    static uint32_t lastLoraSend = 0;
    static uint32_t lastGprsSend = 0;
    uint32_t now = millis();
    
    // LoRaWAN send every UPLINK_INTERVAL_MS
    if (now - lastLoraSend >= UPLINK_INTERVAL_MS) {
        lastLoraSend = now;
        sendLoRaData();
    }
    
    // GPRS send every GPRS_INTERVAL_MS
    if (gprsConnected && (now - lastGprsSend >= GPRS_INTERVAL_MS)) {
        lastGprsSend = now;
        if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
            if (g_gpsData.hasLocation) {
                sendToServerGPRS(g_gpsData.lat, g_gpsData.lng, g_gpsData.alt, g_gpsData.satellites);
            }
            xSemaphoreGive(xGpsMutex);
        }
    }
    
    // Update time display
    static uint32_t lastTimePrint = 0;
    if (now - lastTimePrint >= 5000) {
        lastTimePrint = now;
        
        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            time_t t = time(nullptr);
            if (t > 1700000000L) {
                struct tm *tm = localtime(&t);
                char buf[32];
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
                g_TftPageData[3].rows[1] = "Time: " + String(buf);
            } else {
                g_TftPageData[3].rows[1] = "Time: Not synced";
            }
            xSemaphoreGive(xTftMutex);
        }
        
        time_t now_t = time(nullptr);
        if (now_t > 1700000000L) {
            struct tm *tm = localtime(&now_t);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
            Serial.print(F("[CLOCK] "));
            Serial.println(buf);
        }
    }
    
    // Update TFT
    updateTFT();
    
    // Blink LED based on status
    static uint32_t lastBlink = 0;
    if (now - lastBlink >= 1000) {
        lastBlink = now;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    
    delay(100);
}
