// ============================================
// HELTEC TRACKER V1.1 - ALL-IN-ONE PROGRAM
// GPS + LoRaWAN + WiFi API + IR Receiver + SD Card + TFT Display
// Based on professor's FreeRTOS structure with your features
// ============================================

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <IRremote.hpp>

// ============================================
// KONFIGURASI - UBAH DI SINI SEBELUM UPLOAD
// ============================================
#define WIFI_SSID       "UserAndroid"        // Ubah SSID WiFi
#define WIFI_PASSWORD   "55555550"          // Ubah Password WiFi
#define API_URL         "https://laser-tag-project.vercel.app/api/track"
#define DEVICE_ID       "Heltec-P1"
#define IR_RECEIVE_PIN  5

// ============================================
// LoRaWAN Credentials - GANTI DENGAN MILIK ANDA DARI TTN!
// ============================================
// PENTING: Cek format di TTN Console!
// DevEUI dan JoinEUI harus dalam format LSB (Least Significant Byte)
// AppKey dan NwkKey tetap dalam format MSB (Most Significant Byte)

const uint64_t joinEUI = 0x0000000000000011;  // JoinEUI dari TTN (LSB)
const uint64_t devEUI  = 0x70B3D57ED00739E0;  // DevEUI dari TTN (LSB)

// AppKey dari TTN (MSB)
const uint8_t appKey[] = {
    0xDB, 0x51, 0xEB, 0x2C, 0x50, 0x13, 0x84, 0x82,
    0xE2, 0xFB, 0xD2, 0x64, 0xE7, 0x87, 0x64, 0x27
};

// NwkKey dari TTN (MSB)
const uint8_t nwkKey[] = {
    0x84, 0x0E, 0x4C, 0x4C, 0xD0, 0x87, 0xC8, 0x39,
    0x56, 0x58, 0x04, 0xE0, 0x39, 0xA7, 0x36, 0x16
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
#define RADIO_DIO1_PIN 14  // IRQ
#define RADIO_BUSY_PIN 13  // BUSY

// GPS
#define GPS_TX_PIN 34
#define GPS_RX_PIN 33

// Timezone offset from UTC in hours (WIB = +7, WITA = +8, WIT = +9)
const int8_t TIMEZONE_OFFSET_HOURS = 7;

// SD Card
#define SD_CS 4

// TFT
#define TFT_CS   38
#define TFT_DC   40
#define TFT_RST  39
#define TFT_SCLK 41
#define TFT_MOSI 42

#ifndef LED_BUILTIN
#define LED_BUILTIN 18
#endif

// ============================================
// DISPLAY CONSTANTS
// ============================================
#define TFT_WIDTH        160
#define TFT_HEIGHT       80
#define TFT_MAX_PAGES    3    // LoRa, GPS, IR
#define TFT_ROWS_PER_PAGE 6
#define PAGE_SWITCH_MS   3000  // 3 seconds per page

const int TFT_LEFT_MARGIN  = 3;
const int TFT_TOP_MARGIN   = 2;
const int TFT_LINE_HEIGHT  = 10;

// ============================================
// TIMING & BEHAVIOR
// ============================================
const uint32_t UPLINK_INTERVAL_MS = 30000;  // 30 sec (Fair Use Policy)
const uint8_t  MAX_JOIN_ATTEMPTS  = 10;
const uint32_t JOIN_RETRY_DELAY_MS = 3000;  // 3 seconds (faster join)
const uint32_t SD_WRITE_INTERVAL_MS = 2000;
const size_t   LOG_QUEUE_SIZE = 20;
const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(100);

// RadioLib Region – CHANGE TO YOUR REGION!
const LoRaWANBand_t Region = AS923;  // AS923 untuk Indonesia
const uint8_t subBand = 0;

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

// LoRaWAN Payload Structure (32-byte aligned for LoRaWAN)
// Total: 32 bytes
// | 4 bytes  | 1 byte | 4 bytes | 1 byte | 1 byte | 4 bytes | 4 bytes | 4 bytes | 2 bytes | 1 byte | 2 bytes | 1 byte |
// |---------|--------|---------|--------|--------|---------|---------|---------|---------|--------|--------|--------|
// | addr_id | sub_id | shooter | s_sht | status | lat     | lng     | alt     | battery | sat    | rssi   | snr    |
struct __attribute__((packed)) DataPayload {
    uint32_t address_id;            // Device identifier
    uint8_t  sub_address_id;         // Sub-device ID (IR Command)
    uint32_t shooter_address_id;     // IR Address (shooter ID)
    uint8_t  shooter_sub_address_id; // IR status flag
    uint8_t  status;                 // 0 = inactive, 1 = active (IR hit)
    float    lat;                    // GPS latitude
    float    lng;                    // GPS longitude
    float    alt;                    // GPS altitude
    uint16_t battery_mv;             // Battery voltage in millivolts
    uint8_t  satellites;             // Number of GPS satellites locked
    int16_t  rssi;                   // LoRa RSSI (negative value)
    int8_t   snr;                    // LoRa SNR (dB)
};

// ============================================
// GLOBAL OBJECTS
// ============================================
TinyGPSPlus GPS;
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(TFT_WIDTH, TFT_HEIGHT);

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
SemaphoreHandle_t xIrMutex = NULL;
QueueHandle_t xLogQueue = NULL;

TaskHandle_t xGpsTaskHandle = NULL;
TaskHandle_t xLoraTaskHandle = NULL;
TaskHandle_t xTftTaskHandle = NULL;
TaskHandle_t xSdTaskHandle = NULL;
TaskHandle_t xWifiTaskHandle = NULL;
TaskHandle_t xIrTaskHandle = NULL;

// ============================================
// FORWARD DECLARATIONS
// ============================================
void gpsTask(void *pv);
void loraTask(void *pv);
void tftTask(void *pv);
void sdCardTask(void *pv);
void wifiTask(void *pv);
void irTask(void *pv);
void logToSd(const String &msg);
String formatLogLine();
void sendToAPI(float lat, float lng);
void saveToSDOffline(float lat, float lng, float alt, uint8_t satellites,
                    uint16_t battery, int16_t rssi, float snr, const String &irStatus);
bool uploadFromSD();
String stateDecode(const int16_t result);

// ============================================
// SETUP
// ============================================
void setup()
{
    pinMode(Vext_Ctrl, OUTPUT);
    digitalWrite(Vext_Ctrl, HIGH); // Power on radio, GPS, SD, TFT
    delay(100);
    pinMode(LED_K, OUTPUT);
    digitalWrite(LED_K, HIGH);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);

    Serial.println("\n\n========================================");
    Serial.println("GPS Tracker + IR + LoRaWAN + WiFi + SD");
    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.print("API URL: ");
    Serial.println(API_URL);
    Serial.println("========================================\n");

    // Initialize GPS UART
    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GNSS module powered and Serial1 started.");

    // Initialize IR Receiver
    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
    Serial.println("IR Receiver Ready on Pin 5");

    // Initialize TFT
    tft.initR(INITR_MINI160x80);
    tft.setRotation(1); // Landscape: 160x80
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print("GPS+LoRa+IR+WiFi+SD");
    tft.setCursor(5, 20);
    tft.print(DEVICE_ID);
    tft.setCursor(5, 35);
    tft.print("Initializing...");
    delay(1500);

    // Create mutexes
    xGpsMutex = xSemaphoreCreateMutex();
    xLoraMutex = xSemaphoreCreateMutex();
    xTftMutex = xSemaphoreCreateMutex();
    xWifiMutex = xSemaphoreCreateMutex();
    xIrMutex = xSemaphoreCreateMutex();

    // Create log queue
    xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(String *));
    if (!xLogQueue) {
        Serial.println("❌ Failed to create log queue!");
        while (1) delay(1000);
    }

    // Create tasks
    xTaskCreatePinnedToCore(gpsTask, "GPS", 4096, NULL, 2, &xGpsTaskHandle, 1);
    xTaskCreatePinnedToCore(loraTask, "LoRa", 8192, NULL, 2, &xLoraTaskHandle, 1);
    xTaskCreatePinnedToCore(tftTask, "TFT", 4096, NULL, 1, &xTftTaskHandle, 0);
    xTaskCreatePinnedToCore(sdCardTask, "SD", 4096, NULL, 1, &xSdTaskHandle, 0);
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 4096, NULL, 1, &xWifiTaskHandle, 0);
    xTaskCreatePinnedToCore(irTask, "IR", 2048, NULL, 1, &xIrTaskHandle, 0);

    logToSd("=== System Boot ===");
}

// ============================================
// MAIN LOOP
// ============================================
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

        // Update TFT GPS page with local time
        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE)
        {
            g_TftPageData[1].rows[0] = "GPS Status";
            g_TftPageData[1].rows[1] = hasLoc ? ("Lat: " + String(g_gpsData.lat, 6)) : "Lat: -";
            g_TftPageData[1].rows[2] = hasLoc ? ("Long: " + String(g_gpsData.lng, 6)) : "Long: -";
            g_TftPageData[1].rows[3] = hasLoc ? ("Alt: " + String(g_gpsData.alt, 1) + " m") : "Alt: -";
            g_TftPageData[1].rows[4] = "Sat: " + String(g_gpsData.satellites);

            // Convert UTC to local time
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

// ============================================
// LoRaWAN TASK
// ============================================
void loraTask(void *pv)
{
    // Initialize Radio
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        String err = "Radio init failed: " + stateDecode(state);
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    // Initialize LoRaWAN node
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        String err = "LoRaWAN init failed: " + stateDecode(state);
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    // OTAA Join
    logToSd("Starting OTAA join...");
    Serial.println("Starting OTAA join...");

    uint8_t attempts = 0;
    while (attempts++ < MAX_JOIN_ATTEMPTS)
    {
        String msg = "OTAA attempt " + String(attempts);
        Serial.println(msg);
        logToSd(msg);

        state = node.activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            logToSd("✅ OTAA Join successful!");
            Serial.println("✅ OTAA Join successful!");
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.joined = true;
                g_loraStatus.lastEvent = "JOINED";
                xSemaphoreGive(xLoraMutex);
            }
            break;
        } else {
            String err = "Join failed: " + stateDecode(state);
            Serial.println(err);
            logToSd(err);
            delay(JOIN_RETRY_DELAY_MS);
        }
    }

    if (!g_loraStatus.joined) {
        logToSd("❌ Max join attempts reached.");
        Serial.println("❌ Max join attempts reached.");
        vTaskDelete(NULL);
    }

    // Periodic Uplink
    uint32_t lastUplink = 0;
    for (;;)
    {
        if (millis() - lastUplink >= UPLINK_INTERVAL_MS)
        {
            // Prepare payload with all sensor data
            DataPayload payload;
            payload.address_id = 0x00000000;  // Device ID: 0=P1, 1=P2, 2=P3...
            payload.battery_mv = analogReadMilliVolts(0);

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

            if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
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

            // Serialize payload
            uint8_t buffer[sizeof(DataPayload)];
            memcpy(buffer, &payload, sizeof(DataPayload));

            state = node.sendReceive(buffer, sizeof(buffer));
            String eventStr;
            float snr = 0.0f;
            int16_t rssi = 0;

            if (state < RADIOLIB_ERR_NONE) {
                eventStr = "TX_FAIL";
            } else {
                if (state > 0) {
                    snr = radio.getSNR();
                    rssi = radio.getRSSI();
                    eventStr = "TX+RX_OK";
                } else {
                    eventStr = "TX_OK";
                }
                
                // Reset IR flag after successful transmission
                if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_irStatus.dataReceived = false;
                    xSemaphoreGive(xIrMutex);
                }
            }

            // Update LoRa status
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.lastEvent = eventStr;
                g_loraStatus.snr = snr;
                g_loraStatus.rssi = rssi;
                xSemaphoreGive(xLoraMutex);
            }

            // Update TFT LoRa page
            if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[0].rows[0] = "LoRaWAN Status";
                g_TftPageData[0].rows[1] = "Event: " + eventStr;
                g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
                g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
                g_TftPageData[0].rows[4] = "Batt: " + String(analogReadMilliVolts(0)) + " mV";
                g_TftPageData[0].rows[5] = "Joined: YES";
                xSemaphoreGive(xTftMutex);
            }

            // Log structured data
            logToSd(formatLogLine());

            lastUplink = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// WiFi TASK
// ============================================
void wifiTask(void *pv)
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_wifiStatus.connected = true;
            g_wifiStatus.ip = WiFi.localIP().toString();
            xSemaphoreGive(xWifiMutex);
        }
        Serial.print("IP Address: ");
        Serial.println(g_wifiStatus.ip);
        logToSd("WiFi connected: " + g_wifiStatus.ip);
        
        // Upload data dari SD card jika ada
        Serial.println("[WiFi] Checking for offline data...");
        uploadFromSD();
    } else {
        Serial.println("\nWiFi connection failed");
        logToSd("WiFi connection failed");
    }

    uint32_t lastAPIUpload = 0;
    uint32_t lastSDCheck = 0;
    
    for (;;)
    {
        // Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_wifiStatus.connected = false;
                xSemaphoreGive(xWifiMutex);
            }
            if (millis() - g_wifiStatus.lastReconnect >= 5000) {
                g_wifiStatus.lastReconnect = millis();
                Serial.println("WiFi disconnected, reconnecting...");
                WiFi.reconnect();
            }
        } else {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                if (!g_wifiStatus.connected) {
                    g_wifiStatus.connected = true;
                    Serial.println("WiFi reconnected!");
                    // Upload dari SD saat WiFi tersambung ulang
                    Serial.println("[WiFi] Reconnected - uploading offline data...");
                    uploadFromSD();
                }
                xSemaphoreGive(xWifiMutex);
            }
        }

        // Kirim data GPS setiap 10 detik
        if (g_wifiStatus.connected && millis() - lastAPIUpload >= 10000) {
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                if (g_gpsData.valid && GPS.location.isValid()) {
                    sendToAPI(g_gpsData.lat, g_gpsData.lng);
                }
                xSemaphoreGive(xGpsMutex);
            }
            lastAPIUpload = millis();
        }

        // Cek dan upload data dari SD setiap 30 detik
        if (g_wifiStatus.connected && millis() - lastSDCheck >= 30000) {
            uploadFromSD();
            lastSDCheck = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

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
                    xSemaphoreGive(xIrMutex);
                }
                
                Serial.printf("IR Received | Addr: 0x%04X | Cmd: 0x%02X\n", 
                             g_irStatus.address, g_irStatus.command);
                logToSd("IR: Addr=" + String(g_irStatus.address, HEX) + 
                       " Cmd=" + String(g_irStatus.command, HEX));
            }
            IrReceiver.resume();
        }

        // Update TFT IR page
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
            
            // Status bar
            framebuffer.setCursor(2, 2);
            bool wifiOk = false, loraOk = false;
            if (xSemaphoreTake(xWifiMutex, 10) == pdTRUE) {
                wifiOk = g_wifiStatus.connected;
                xSemaphoreGive(xWifiMutex);
            }
            if (xSemaphoreTake(xLoraMutex, 10) == pdTRUE) {
                loraOk = g_loraStatus.joined;
                xSemaphoreGive(xLoraMutex);
            }
            framebuffer.printf("W:%s L:%s", wifiOk ? "OK" : "X", loraOk ? "OK" : "X");
            
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
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// SD CARD LOGGING TASK
// ============================================
void sdCardTask(void *pv)
{
    // Initialize SD
    if (!SD.begin(SD_CS, SPI, 8000000)) {
        Serial.println("SD Mount Failed!");
        vTaskDelete(NULL);
    }

    Serial.println("SD Card initialized successfully");

    // Delete old log file on boot
    if (SD.exists("/tracker.log")) {
        SD.remove("/tracker.log");
        Serial.println("Old tracker.log deleted.");
    }

    File file;
    uint32_t lastWrite = 0;

    for (;;)
    {
        uint32_t now = millis();
        if (now - lastWrite >= SD_WRITE_INTERVAL_MS) {
            file = SD.open("/tracker.log", FILE_APPEND);
            if (file) {
                String *msg;
                while (xQueueReceive(xLogQueue, &msg, 0) == pdTRUE) {
                    file.println(*msg);
                    delete msg;
                }
                file.close();
                lastWrite = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// HELPER FUNCTIONS
// ============================================
void logToSd(const String &msg)
{
    String *copy = new String(msg);
    if (xQueueSendToBack(xLogQueue, &copy, pdMS_TO_TICKS(10)) != pdTRUE) {
        delete copy;
    }
}

// ============================================
void saveToSDOffline(float lat, float lng, float alt, uint8_t satellites,
                    uint16_t battery, int16_t rssi, float snr, const String &irStatus)
{
    if (!SD.begin(SD_CS, SPI, 8000000)) {
        return;
    }
    
    File file = SD.open("/offline_queue.csv", FILE_APPEND);
    if (!file) {
        return;
    }
    
    String line = String(millis()) + "," +
                  String(lat, 6) + "," +
                  String(lng, 6) + "," +
                  String(alt, 1) + "," +
                  String(satellites) + "," +
                  String(battery) + "," +
                  String(rssi) + "," +
                  String(snr, 1) + "," +
                  irStatus;
    
    file.println(line);
    file.close();
}

bool uploadFromSD()
{
    if (!SD.begin(SD_CS, SPI, 8000000)) {
        return false;
    }
    
    if (!SD.exists("/offline_queue.csv")) {
        return false;
    }
    
    File file = SD.open("/offline_queue.csv", FILE_READ);
    if (!file) {
        return false;
    }
    
    int uploadCount = 0;
    String tempFileContent = "";
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() < 10) continue;
        
        // Parse CSV: millis,lat,lng,alt,satellites,battery,rssi,snr,irStatus
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
        
        // Kirim ke API
        HTTPClient http;
        http.begin(API_URL);
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
            tempFileContent += line + "\n";
        }
        
        delay(100);
    }
    
    file.close();
    
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
}
    
    if (!SD.exists("/offline_queue.csv")) {
        return false;
    }
    
    File file = SD.open("/offline_queue.csv", FILE_READ);
    if (!file) {
        Serial.println("[SD-UPLOAD] Failed to open queue file");
        return false;
    }
    
    int uploadCount = 0;
    String tempFileContent = "";
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() < 10) continue;
        
        // Parse CSV: millis,lat,lng,alt,satellites,battery,rssi,snr,irStatus
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
        
        // Kirim ke API
        HTTPClient http;
        http.begin(API_URL);
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
            tempFileContent += line + "\n";
        }
        
        delay(100);
    }
    
    file.close();
    
    if (uploadCount > 0) {
        File tempFile = SD.open("/offline_queue.csv", FILE_WRITE);
        if (tempFile) {
            tempFile.print(tempFileContent);
            tempFile.close();
            Serial.println("[SD-UPLOAD] Queue updated with failed entries");
        }
        return false;
    } else {
        SD.remove("/offline_queue.csv");
        Serial.println("[SD-UPLOAD] Queue cleared - all uploaded");
        return true;
    }
}

void sendToAPI(float lat, float lng)
{
    String irStatus = "-";
    uint16_t battery = 0;
    uint8_t satellites = 0;
    float alt = 0.0f;
    int16_t rssi = 0;
    float snr = 0.0f;

    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_irStatus.dataReceived) {
            irStatus = "HIT: 0x" + String(g_irStatus.address, HEX) + "-0x" + String(g_irStatus.command, HEX);
        }
        xSemaphoreGive(xIrMutex);
    }

    battery = analogReadMilliVolts(0);

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
    http.begin(API_URL);
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

    if (httpCode != 200 && httpCode != 201) {
        saveToSDOffline(lat, lng, alt, satellites, battery, rssi, snr, irStatus);
    }
}

String formatLogLine()
{
    char timeBuf[9];
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

    String event = "IDLE";
    float snr = 0.0f;
    int16_t rssi = 0;
    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        event = g_loraStatus.lastEvent;
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

    char logLine[150];
    snprintf(logLine, sizeof(logLine),
             "%s,%s,%.1f,%d,%s,%s,%s",
             timeBuf, event.c_str(), snr, rssi, latBuf, lonBuf, altBuf);
    return String(logLine);
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
