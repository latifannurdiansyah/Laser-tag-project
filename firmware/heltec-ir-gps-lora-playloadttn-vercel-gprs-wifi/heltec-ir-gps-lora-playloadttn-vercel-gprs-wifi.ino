// ============================================
// HELTEC TRACKER V1.1 - ALL-IN-ONE PROGRAM
// GPS + LoRaWAN + WiFi + GPRS + IR Receiver + SD Card + TFT Display
//
// FIX v3:
//   - TFT pages normal (auto slide 3 detik)
//   - Hapus debug GPS di Serial Monitor
//   - WiFi selalu kirim ke Vercel API (tanpa syarat GPS valid)
//   - xSpiMutex untuk TFT + SD
//   - locValid disimpan di struct GpsData
// ============================================

#define ENABLE_WIFI    1
#define ENABLE_GPRS    1
#define ENABLE_LORAWAN 1
#define ENABLE_SD      1

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <IRremote.hpp>
#include "esp_task_wdt.h"

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
// KONFIGURASI
// ============================================
#define WIFI_SSID          "UserAndroid"
#define WIFI_PASSWORD      "55555550"
#define WIFI_API_URL       "https://laser-tag-project.vercel.app/api/track"

#define GPRS_APN           "indosatgprs"
#define GPRS_USER          "indosat"
#define GPRS_PASS          "indosat"
#define SIM_PIN            ""
#define THINGSPEAK_URL     "api.thingspeak.com"
#define THINGSPEAK_PORT    80
#define THINGSPEAK_API_KEY "N6ATMD4JVI90HTHH"

#define DEVICE_ID          "Heltec-P1"
#define IR_RECEIVE_PIN     5

// ============================================
// LoRaWAN Credentials
// ============================================
const uint64_t joinEUI = 0x0000000000000003;
const uint64_t devEUI  = 0x70B3D57ED0075AC0;
const uint8_t appKey[] = { 0x48,0xF0,0x3F,0xDD,0x9C,0x5A,0x9E,0xBA,0x42,0x83,0x01,0x1D,0x7D,0xBB,0xF3,0xF8 };
const uint8_t nwkKey[] = { 0x9B,0xF6,0x12,0xF4,0xAA,0x33,0xDB,0x73,0xAA,0x1B,0x7A,0xC6,0x4D,0x70,0x02,0x26 };

// ============================================
// PIN DEFINITIONS
// ============================================
#define Vext_Ctrl      3
#define LED_K          21
#define RADIO_SCLK_PIN 9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10
#define RADIO_CS_PIN   8
#define RADIO_RST_PIN  12
#define RADIO_DIO1_PIN 14
#define RADIO_BUSY_PIN 13
#define GPS_TX_PIN     34
#define GPS_RX_PIN     33
#define GPRS_TX_PIN    17
#define GPRS_RX_PIN    16
#define GPRS_RST_PIN   15
#define SD_CS          4
#define TFT_CS         38
#define TFT_DC         40
#define TFT_RST        39
#define TFT_SCLK       41
#define TFT_MOSI       42

// ============================================
// TIMING
// ============================================
const int8_t   TIMEZONE_OFFSET_HOURS = 7;
const uint32_t UPLINK_INTERVAL_MS    = 10000;
const uint32_t WIFI_UPLOAD_INTERVAL  = 10000;
const uint32_t GPRS_INTERVAL_MS      = 5000;
const uint8_t  MAX_JOIN_ATTEMPTS     = 5;
const uint32_t JOIN_RETRY_DELAY_MS   = 2000;
const uint32_t SD_WRITE_INTERVAL_MS  = 2000;
const size_t   LOG_QUEUE_SIZE        = 20;
const TickType_t MUTEX_TIMEOUT       = pdMS_TO_TICKS(100);
const LoRaWANBand_t Region           = AS923;
const uint8_t subBand                = 0;

// ============================================
// DISPLAY
// ============================================
#define TFT_WIDTH         160
#define TFT_HEIGHT         80
#define TFT_MAX_PAGES       3   // 0:GPS  1:LoRa  2:IR
#define TFT_ROWS_PER_PAGE   6
#define PAGE_SWITCH_MS   3000
const int TFT_LEFT_MARGIN = 3;
const int TFT_TOP_MARGIN  = 2;
const int TFT_LINE_HEIGHT = 10;

// ============================================
// DATA STRUCTURES
// ============================================
struct GpsData {
    bool    valid;
    bool    locValid;
    float   lat, lng, alt;
    uint8_t satellites;
    uint8_t hour, minute, second;
};

struct LoraStatus {
    String  lastEvent;
    int16_t rssi;
    float   snr;
    bool    joined;
};

struct WiFiStatus {
    bool          connected;
    String        ip;
    unsigned long lastReconnect;
    int           lastHttpCode;
    uint32_t      uploadOK;
    uint32_t      uploadFail;
};

struct GprsStatus {
    bool     connected;
    String   ip;
    String   lastEvent;
};

struct IRStatus {
    bool          dataReceived;
    uint16_t      address;
    uint8_t       command;
    unsigned long lastTime;
};

struct TftPageData {
    String rows[TFT_ROWS_PER_PAGE];
};

struct __attribute__((packed)) DataPayload {
    uint32_t address_id;
    uint8_t  sub_address_id;
    uint32_t shooter_address_id;
    uint8_t  shooter_sub_address_id;
    uint8_t  status;
    float    lat, lng, alt;
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
GFXcanvas16 framebuffer(TFT_WIDTH, TFT_HEIGHT);

#if ENABLE_GPRS
HardwareSerial sim900ASerial(2);
TinyGsm        modem(sim900ASerial);
TinyGsmClient  gprsClient(modem);
#endif

// ============================================
// GLOBAL STATE
// ============================================
GpsData g_gps = { false, false, 0,0,0, 0, 0,0,0 };

LoraStatus g_lora = { "IDLE", 0, 0.0f, false };

WiFiStatus g_wifi = { false, "N/A", 0, 0, 0, 0 };

GprsStatus g_gprs = { false, "N/A", "IDLE" };

IRStatus g_ir = { false, 0, 0, 0 };

// TFT pages — baris 0 = judul halaman
TftPageData g_pages[TFT_MAX_PAGES] = {
    { {"LoRaWAN", "Status: IDLE", "RSSI: -", "SNR: -", "Joined: NO", "-"} },
    { {"GPS", "Lat: -", "Lng: -", "Alt: -", "Sat: 0", "Time: --:--:--"} },
    { {"IR Receiver", "Waiting...", "-", "-", "-", "-"} },
    { {"WiFi", "Status: -", "IP: N/A", "Last: -", "OK: 0", "Fail: 0"} }
};

bool     sdInitialized    = false;
bool     g_sendImmediately = false;
bool     g_irPending      = false;
uint16_t g_pendingAddress = 0;
uint8_t  g_pendingCommand = 0;

// ============================================
// RTOS HANDLES
// ============================================
SemaphoreHandle_t xGpsMutex  = NULL;
SemaphoreHandle_t xLoraMutex = NULL;
SemaphoreHandle_t xTftMutex  = NULL;
SemaphoreHandle_t xWifiMutex = NULL;
SemaphoreHandle_t xGprsMutex = NULL;
SemaphoreHandle_t xIrMutex   = NULL;
SemaphoreHandle_t xSpiMutex  = NULL;
QueueHandle_t     xLogQueue  = NULL;

TaskHandle_t hGps=NULL, hLora=NULL, hTft=NULL, hSd=NULL;
TaskHandle_t hWifi=NULL, hGprs=NULL, hIr=NULL;

// ============================================
// FORWARD DECLARATIONS
// ============================================
void gpsTask(void*); void loraTask(void*); void tftTask(void*);
void sdCardTask(void*); void wifiTask(void*); void gprsTask(void*); void irTask(void*);
void logToSd(const char*);
void sendToWiFiAPI(float lat, float lng);
void sendToGPRS(float lat, float lng);
void saveToSDOffline(float,float,float,uint8_t,uint16_t,int16_t,float,const char*);
bool uploadFromSD();
String stateDecode(int16_t);
void updateTftWifiPage();

// ============================================
// SETUP
// ============================================
void setup()
{
    pinMode(Vext_Ctrl, OUTPUT); digitalWrite(Vext_Ctrl, HIGH); delay(100);
    pinMode(LED_K, OUTPUT);     digitalWrite(LED_K, HIGH);
#ifndef LED_BUILTIN
#define LED_BUILTIN 18
#endif
    pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, LOW);

#if ENABLE_GPRS
    pinMode(GPRS_RST_PIN, OUTPUT);
    digitalWrite(GPRS_RST_PIN, HIGH); delay(200);
    digitalWrite(GPRS_RST_PIN, LOW);  delay(200);
    digitalWrite(GPRS_RST_PIN, HIGH); delay(1000);
#endif

    Serial.begin(115200);
    while (!Serial && millis() < 3000) delay(10);
    Serial.printf("\n[BOOT] %s WiFi:%d GPRS:%d LoRa:%d SD:%d\n",
        DEVICE_ID, ENABLE_WIFI, ENABLE_GPRS, ENABLE_LORAWAN, ENABLE_SD);

    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#if ENABLE_GPRS
    sim900ASerial.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
    delay(1000);
#endif

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    // Init TFT
    tft.initR(INITR_MINI160x80);
    tft.setRotation(1);
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_CYAN);
    tft.setTextSize(1);
    tft.setCursor(10, 10); tft.print("Heltec Tracker");
    tft.setCursor(10, 25); tft.print(DEVICE_ID);
    tft.setCursor(10, 40); tft.print("Booting...");
    delay(1500);

    // Buat mutex
    xGpsMutex  = xSemaphoreCreateMutex();
    xLoraMutex = xSemaphoreCreateMutex();
    xTftMutex  = xSemaphoreCreateMutex();
    xWifiMutex = xSemaphoreCreateMutex();
    xGprsMutex = xSemaphoreCreateMutex();
    xIrMutex   = xSemaphoreCreateMutex();
    xSpiMutex  = xSemaphoreCreateMutex();
    xLogQueue  = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char*));
    if (!xLogQueue) { Serial.println("[BOOT] Queue fail!"); while(1) delay(1000); }

    // CPU 1: GPS, IR, LoRa, WiFi, GPRS
    xTaskCreatePinnedToCore(gpsTask,  "GPS",  12288, NULL, 2, &hGps,  1);
    xTaskCreatePinnedToCore(irTask,   "IR",    4096, NULL, 1, &hIr,   1);
#if ENABLE_LORAWAN
    xTaskCreatePinnedToCore(loraTask, "LoRa", 16384, NULL, 2, &hLora, 1);
#endif
#if ENABLE_WIFI
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 65536, NULL, 1, &hWifi, 1);
#endif
#if ENABLE_GPRS
    xTaskCreatePinnedToCore(gprsTask, "GPRS", 65536, NULL, 1, &hGprs, 1);
#endif

    // CPU 0: TFT, SD
    xTaskCreatePinnedToCore(tftTask,  "TFT",  65536, NULL, 1, &hTft, 0);
#if ENABLE_SD
    xTaskCreatePinnedToCore(sdCardTask, "SD", 16384, NULL, 1, &hSd,  0);
#endif
}

void loop() { delay(1000); }

// ============================================
// GPS TASK (CPU 1)
// Satu-satunya task yang akses GPS object
// ============================================
void gpsTask(void *pv)
{
    for (;;)
    {
        int avail = Serial1.available();
        for (int i = 0; i < avail; i++) GPS.encode(Serial1.read());

        bool hasTime = GPS.time.isValid();
        bool hasLoc  = GPS.location.isValid();

        if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_gps.valid    = hasTime || hasLoc;
            g_gps.locValid = hasLoc;
            g_gps.satellites = GPS.satellites.value();
            if (hasLoc) {
                g_gps.lat = GPS.location.lat();
                g_gps.lng = GPS.location.lng();
                g_gps.alt = GPS.altitude.meters();
            }
            if (hasTime) {
                g_gps.hour   = GPS.time.hour();
                g_gps.minute = GPS.time.minute();
                g_gps.second = GPS.time.second();
            }
            xSemaphoreGive(xGpsMutex);
        }

        // Update TFT GPS page
        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            int8_t dh = 0;
            if (hasTime) {
                dh = (int8_t)(g_gps.hour + TIMEZONE_OFFSET_HOURS);
                if (dh >= 24) dh -= 24;
                if (dh < 0)   dh += 24;
            }
            char tb[12];
            snprintf(tb, sizeof(tb), "%02d:%02d:%02d",
                     hasTime ? dh           : 0,
                     hasTime ? g_gps.minute : 0,
                     hasTime ? g_gps.second : 0);
            g_pages[0].rows[0] = "GPS";  // Slide 1
            g_pages[0].rows[1] = hasLoc ? ("Lat:" + String(g_gps.lat, 5))  : "Lat: -";
            g_pages[0].rows[2] = hasLoc ? ("Lng:" + String(g_gps.lng, 5))  : "Lng: -";
            g_pages[0].rows[3] = hasLoc ? ("Alt:" + String(g_gps.alt, 1) + "m") : "Alt: -";
            g_pages[0].rows[4] = "Sat:" + String(g_gps.satellites);
            g_pages[0].rows[5] = "Jam:" + String(tb);  // Waktu sekarang
            xSemaphoreGive(xTftMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================
// LORAWAN TASK (CPU 1)
// ============================================
#if ENABLE_LORAWAN
void loraTask(void *pv)
{
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    delay(100);

    auto setLoraPage = [](const char* ev, int16_t rssi, float snr, bool joined) {
        if (xSemaphoreTake(xTftMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_pages[1].rows[0] = "LoRaWAN";  // Slide 2
            g_pages[1].rows[1] = String("Ev:") + ev;
            g_pages[1].rows[2] = "RSSI:" + String(rssi) + "dBm";
            g_pages[1].rows[3] = "SNR:" + String(snr, 1) + "dB";
            g_pages[1].rows[4] = joined ? "Joined:YES" : "Joined:NO";
            g_pages[1].rows[5] = "-";
            xSemaphoreGive(xTftMutex);
        }
    };

    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Radio fail: %s\n", stateDecode(state).c_str());
        setLoraPage("RADIO_FAIL", 0, 0, false);
        vTaskDelete(NULL);
    }
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Node fail: %s\n", stateDecode(state).c_str());
        setLoraPage("INIT_FAIL", 0, 0, false);
        vTaskDelete(NULL);
    }

    for (uint8_t att = 1; att <= MAX_JOIN_ATTEMPTS; att++) {
        Serial.printf("[LoRa] Join %d/%d\n", att, MAX_JOIN_ATTEMPTS);
        setLoraPage(("JOIN " + String(att)).c_str(), 0, 0, false);
        state = node.activateOTAA();
        vTaskDelay(pdMS_TO_TICKS(10));
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.println("[LoRa] Joined!");
            logToSd("LoRa join OK");
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_lora.joined = true; g_lora.lastEvent = "JOINED";
                xSemaphoreGive(xLoraMutex);
            }
            setLoraPage("JOINED", 0, 0, true);
            break;
        }
        Serial.printf("[LoRa] Fail: %s\n", stateDecode(state).c_str());
        vTaskDelay(pdMS_TO_TICKS(JOIN_RETRY_DELAY_MS));
    }

    if (!g_lora.joined) {
        setLoraPage("NO_JOIN", 0, 0, false);
        vTaskDelete(NULL);
    }

    uint32_t lastUplink = 0;
    for (;;) {
        if (g_sendImmediately || millis() - lastUplink >= UPLINK_INTERVAL_MS) {
            g_sendImmediately = false;
            DataPayload pl = {};
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                pl.satellites = g_gps.satellites;
                pl.lat = g_gps.lat; pl.lng = g_gps.lng; pl.alt = g_gps.alt;
                xSemaphoreGive(xGpsMutex);
            }
            if (g_irPending) {
                pl.sub_address_id = g_pendingCommand;
                pl.shooter_address_id = g_pendingAddress;
                pl.shooter_sub_address_id = 1; pl.status = 1;
                g_irPending = false;
            } else if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                pl.sub_address_id         = g_ir.dataReceived ? g_ir.command : 0;
                pl.shooter_address_id     = g_ir.dataReceived ? g_ir.address : 0;
                pl.shooter_sub_address_id = g_ir.dataReceived ? 1 : 0;
                pl.status                 = g_ir.dataReceived ? 1 : 0;
                xSemaphoreGive(xIrMutex);
            }
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                pl.rssi = g_lora.rssi; pl.snr = (int8_t)(g_lora.snr + 0.5f);
                xSemaphoreGive(xLoraMutex);
            }

            uint8_t buf[sizeof(DataPayload)];
            memcpy(buf, &pl, sizeof(DataPayload));
            state = node.sendReceive(buf, sizeof(buf));

            String ev; float snr = 0; int16_t rssi = 0;
            if (state < RADIOLIB_ERR_NONE) {
                ev = "TX_FAIL";
                Serial.println("[LoRa] TX FAIL");
            } else {
                if (state > 0) { snr = radio.getSNR(); rssi = radio.getRSSI(); ev = "TX+RX"; }
                else ev = "TX_OK";
                Serial.printf("[LoRa] TX OK rssi:%d snr:%.1f\n", rssi, snr);
                if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_ir.dataReceived = false; xSemaphoreGive(xIrMutex);
                }
            }
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_lora.lastEvent = ev; g_lora.snr = snr; g_lora.rssi = rssi;
                xSemaphoreGive(xLoraMutex);
            }
            setLoraPage(ev.c_str(), rssi, snr, true);
            lastUplink = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================
// WIFI TASK (CPU 1)
// Selalu kirim ke Vercel API, retry 3x
// ============================================
#if ENABLE_WIFI
void wifiTask(void *pv)
{
    Serial.printf("[WiFi] Connecting %s\n", WIFI_SSID);

    // WiFi task: menghubungkan dan upload data
    // Note: WiFi status ditampilkan di status bar, tidak perlu slide terpisah

    WiFi.disconnect(true);
    vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(300));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    setWifiPage("Connecting...", "...");

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000)
        vTaskDelay(pdMS_TO_TICKS(500));

    if (WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        Serial.printf("[WiFi] Connected IP:%s\n", ip.c_str());
        setWifiPage("Connected", ip.c_str());
        if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_wifi.connected = true; g_wifi.ip = ip;
            xSemaphoreGive(xWifiMutex);
        }
        uploadFromSD();
    } else {
        Serial.println("[WiFi] Connect failed, will retry");
        setWifiPage("Failed", "N/A");
    }

    uint32_t lastUpload = 0;
    for (;;) {
        // Reconnect
        if (WiFi.status() != WL_CONNECTED) {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_wifi.connected = false; xSemaphoreGive(xWifiMutex);
            }
            setWifiPage("Disconnected", "N/A");
            if (millis() - g_wifi.lastReconnect >= 10000) {
                g_wifi.lastReconnect = millis();
                Serial.println("[WiFi] Reconnecting...");
                setWifiPage("Reconnecting", "...");
                WiFi.disconnect(true);
                vTaskDelay(pdMS_TO_TICKS(300));
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                unsigned long tw = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - tw < 15000)
                    vTaskDelay(pdMS_TO_TICKS(500));
                if (WiFi.status() == WL_CONNECTED) {
                    String ip = WiFi.localIP().toString();
                    Serial.printf("[WiFi] Reconnected IP:%s\n", ip.c_str());
                    setWifiPage("Connected", ip.c_str());
                    if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                        g_wifi.connected = true; g_wifi.ip = ip;
                        xSemaphoreGive(xWifiMutex);
                    }
                } else {
                    Serial.println("[WiFi] Reconnect fail");
                    setWifiPage("Retry fail", "N/A");
                }
            }
        } else {
            if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
                if (!g_wifi.connected) {
                    g_wifi.connected = true;
                    g_wifi.ip = WiFi.localIP().toString();
                }
                xSemaphoreGive(xWifiMutex);
            }
        }

        // Upload setiap 10 detik — jika WiFi, LoRa, atau GPRS terhubung
        bool canUpload = false;
#if ENABLE_WIFI
        if (g_wifi.connected) canUpload = true;
#endif
#if ENABLE_LORAWAN
        if (g_lora.joined) canUpload = true;
#endif
#if ENABLE_GPRS
        if (g_gprs.connected) canUpload = true;
#endif

        if (canUpload && millis() - lastUpload >= WIFI_UPLOAD_INTERVAL) {
            float lat = 0, lng = 0;
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                lat = g_gps.lat; lng = g_gps.lng;
                xSemaphoreGive(xGpsMutex);
            }
            sendToWiFiAPI(lat, lng);
            lastUpload = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sendToWiFiAPI(float lat, float lng)
{
    char    irBuf[32]  = "-";
    float   alt        = 0;
    uint8_t satellites = 0;
    int16_t rssi       = 0;
    float   snr        = 0;
    uint16_t irAddr    = 0;
    uint8_t  irCmd     = 0;

    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_ir.dataReceived) {
            snprintf(irBuf, sizeof(irBuf), "HIT:0x%X-0x%X", g_ir.address, g_ir.command);
            irAddr = g_ir.address; irCmd = g_ir.command;
        }
        xSemaphoreGive(xIrMutex);
    }
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        alt = g_gps.alt; satellites = g_gps.satellites;
        xSemaphoreGive(xGpsMutex);
    }
    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        rssi = g_lora.rssi; snr = g_lora.snr;
        xSemaphoreGive(xLoraMutex);
    }

    char payload[400];
    snprintf(payload, sizeof(payload),
        "{\"source\":\"wifi\",\"id\":\"%s\","
        "\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.1f,"
        "\"irStatus\":\"%s\",\"address\":%u,\"command\":%u,"
        "\"battery\":0,\"satellites\":%u,\"rssi\":%d,\"snr\":%.1f}",
        DEVICE_ID, lat, lng, alt, irBuf, irAddr, irCmd, satellites, rssi, snr);

    Serial.printf("[WiFi] POST %s\n", payload);

    int  code    = -1;
    bool success = false;

    for (int i = 1; i <= 3 && !success; i++) {
        if (WiFi.status() != WL_CONNECTED) break;
        HTTPClient http;
        http.begin(WIFI_API_URL);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000);
        code = http.POST(payload);
        if (code == 200 || code == 201) {
            success = true;
            Serial.printf("[WiFi] OK code:%d\n", code);
        } else {
            Serial.printf("[WiFi] Fail attempt %d code:%d\n", i, code);
            if (i < 3) vTaskDelay(pdMS_TO_TICKS(2000));
        }
        http.end();
    }

    // Update stats dan TFT WiFi page
    if (xSemaphoreTake(xWifiMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_wifi.lastHttpCode = code;
        if (success) g_wifi.uploadOK++; else g_wifi.uploadFail++;
        xSemaphoreGive(xWifiMutex);
    }
    // Note: WiFi status ditampilkan di status bar, tidak perlu update TFT di sini

    // SELALU simpan ke SD Card - terlepas dari upload success atau gagal
    saveToSDOffline(lat, lng, alt, satellites, 0, rssi, snr, irBuf);
}
#endif

// ============================================
// GPRS TASK (CPU 1)
// ============================================
#if ENABLE_GPRS
void gprsTask(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(5000)); // Tunggu WiFi selesai init
    Serial.println("[GPRS] Init...");
    if (!modem.begin()) { Serial.println("[GPRS] Modem fail"); vTaskDelete(NULL); }
    vTaskDelay(pdMS_TO_TICKS(500));

    bool connected = false;
    for (int r = 0; r < 5; r++) {
        Serial.printf("[GPRS] APN attempt %d/5\n", r+1);
        if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
            connected = true;
            String ip = modem.getLocalIP();
            Serial.println("[GPRS] IP=" + ip);
            if (xSemaphoreTake(xGprsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_gprs.connected = true; g_gprs.ip = ip; g_gprs.lastEvent = "OK";
                xSemaphoreGive(xGprsMutex);
            }
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    if (!connected) { Serial.println("[GPRS] Failed"); vTaskDelete(NULL); }

    uint32_t lastSend = 0;
    for (;;) {
        if (!modem.isGprsConnected()) {
            if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
                Serial.println("[GPRS] Reconnected");
                if (xSemaphoreTake(xGprsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_gprs.connected = true; xSemaphoreGive(xGprsMutex);
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(5000)); continue;
            }
        }
        if (millis() - lastSend >= GPRS_INTERVAL_MS) {
            float lat = 0, lng = 0; bool locOk = false;
            if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
                lat = g_gps.lat; lng = g_gps.lng; locOk = g_gps.locValid;
                xSemaphoreGive(xGpsMutex);
            }
            if (locOk) sendToGPRS(lat, lng);
            lastSend = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void sendToGPRS(float lat, float lng)
{
    char irBuf[32] = "-";
    float alt = 0; uint8_t sat = 0; int16_t rssi = 0; float snr = 0;
    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_ir.dataReceived) snprintf(irBuf, 32, "HIT:0x%X-0x%X", g_ir.address, g_ir.command);
        xSemaphoreGive(xIrMutex);
    }
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        alt = g_gps.alt; sat = g_gps.satellites; xSemaphoreGive(xGpsMutex);
    }
    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        rssi = g_lora.rssi; snr = g_lora.snr; xSemaphoreGive(xLoraMutex);
    }
    String url = String("/update?api_key=") + THINGSPEAK_API_KEY
               + "&field1=" + String(lat,6) + "&field2=" + String(lng,6)
               + "&field3=" + String(alt,1) + "&field4=" + String(sat)
               + "&field5=" + String(irBuf) + "&field6=" + String(rssi)
               + "&field7=" + String(snr,1);
    if (gprsClient.connect(THINGSPEAK_URL, THINGSPEAK_PORT)) {
        gprsClient.printf("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                          url.c_str(), THINGSPEAK_URL);
        unsigned long t = millis();
        while (!gprsClient.available() && millis()-t < 10000) vTaskDelay(pdMS_TO_TICKS(10));
        while (gprsClient.available()) { gprsClient.read(); vTaskDelay(1); }
        gprsClient.stop();
        Serial.println("[GPRS] ThingSpeak sent");
    } else {
        Serial.println("[GPRS] Connect fail");
    }
}
#endif

// ============================================
// IR TASK (CPU 1)
// ============================================
void irTask(void *pv)
{
    for (;;) {
        if (IrReceiver.decode()) {
            if (IrReceiver.decodedIRData.protocol == NEC) {
                uint16_t addr = IrReceiver.decodedIRData.address;
                uint8_t  cmd  = IrReceiver.decodedIRData.command;
                if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                    g_ir.dataReceived = true;
                    g_ir.address      = addr;
                    g_ir.command      = cmd;
                    g_ir.lastTime     = millis();
                    g_irPending       = true;
                    g_pendingAddress  = addr;
                    g_pendingCommand  = cmd;
                    g_sendImmediately = true;
                    xSemaphoreGive(xIrMutex);
                }
                Serial.printf("[IR] 0x%04X / 0x%02X\n", addr, cmd);
                char buf[32]; snprintf(buf, 32, "IR:0x%04X,0x%02X", addr, cmd);
                logToSd(buf);
            }
            IrReceiver.resume();
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_pages[2].rows[0] = "IR Receiver";
                if (g_ir.dataReceived) {
                    g_pages[2].rows[1] = "Proto: NEC";
                    g_pages[2].rows[2] = "Addr:0x" + String(g_ir.address, HEX);
                    g_pages[2].rows[3] = "Cmd:0x"  + String(g_ir.command, HEX);
                    g_pages[2].rows[4] = "+" + String((millis()-g_ir.lastTime)/1000) + "s ago";
                    g_pages[2].rows[5] = "-";
                } else {
                    g_pages[2].rows[1] = "Waiting...";
                    g_pages[2].rows[2] = g_pages[2].rows[3] = "-";
                    g_pages[2].rows[4] = g_pages[2].rows[5] = "-";
                }
                xSemaphoreGive(xIrMutex);
            }
            xSemaphoreGive(xTftMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// TFT TASK (CPU 0)
// Auto-slide tiap PAGE_SWITCH_MS
// Row 0 = judul halaman (warna cyan), row 1-5 = data (putih)
// ============================================
void tftTask(void *pv)
{
    esp_task_wdt_add(NULL);

    // Label halaman sesuai index: GPS, LoRa, IR
    const char* pageNames[TFT_MAX_PAGES] = { "GPS", "LoRaWAN", "IR" };
    const uint16_t titleColors[TFT_MAX_PAGES] = {
        ST7735_GREEN, ST7735_YELLOW, ST7735_MAGENTA
    };

    uint32_t lastSwitch = millis();
    uint8_t  page       = 0;

    for (;;)
    {
        esp_task_wdt_reset();

        // Auto-slide
        if (millis() - lastSwitch >= PAGE_SWITCH_MS) {
            page = (page + 1) % TFT_MAX_PAGES;
            lastSwitch = millis();
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {

            framebuffer.fillScreen(ST7735_BLACK);

            // ── Status bar atas: W:OK L:OK G:OK S:OK ─────────────────────
            framebuffer.setTextSize(1);
            framebuffer.setTextColor(ST7735_WHITE);
            framebuffer.setCursor(0, 0);

            bool wOk = false, lOk = false, gOk = false, sOk = false;
#if ENABLE_WIFI
            if (xSemaphoreTake(xWifiMutex, 10) == pdTRUE) { wOk = g_wifi.connected; xSemaphoreGive(xWifiMutex); }
#endif
#if ENABLE_LORAWAN
            if (xSemaphoreTake(xLoraMutex, 10) == pdTRUE) { lOk = g_lora.joined; xSemaphoreGive(xLoraMutex); }
#endif
#if ENABLE_GPRS
            if (xSemaphoreTake(xGprsMutex, 10) == pdTRUE) { gOk = g_gprs.connected; xSemaphoreGive(xGprsMutex); }
#endif
#if ENABLE_SD
            sOk = sdInitialized;
#endif

            // Format: W:OK L:OK G:OK S:OK
            framebuffer.setTextColor(wOk ? ST7735_GREEN : ST7735_RED); framebuffer.print("W:");
            framebuffer.setTextColor(wOk ? ST7735_GREEN : ST7735_RED); framebuffer.print(wOk ? "OK " : "X  ");
            framebuffer.setTextColor(lOk ? ST7735_GREEN : ST7735_RED); framebuffer.print("L:");
            framebuffer.setTextColor(lOk ? ST7735_GREEN : ST7735_RED); framebuffer.print(lOk ? "OK " : "X  ");
            framebuffer.setTextColor(gOk ? ST7735_GREEN : ST7735_RED); framebuffer.print("G:");
            framebuffer.setTextColor(gOk ? ST7735_GREEN : ST7735_RED); framebuffer.print(gOk ? "OK " : "X  ");
            framebuffer.setTextColor(sOk ? ST7735_GREEN : ST7735_RED); framebuffer.print("S:");
            framebuffer.setTextColor(sOk ? ST7735_GREEN : ST7735_RED); framebuffer.print(sOk ? "OK" : "X");

            // Nomor halaman di kanan
            framebuffer.setTextColor(ST7735_WHITE);
            framebuffer.setCursor(TFT_WIDTH - 24, 0);
            framebuffer.printf("%d/%d", page + 1, TFT_MAX_PAGES);

            // ── Garis pemisah ─────────────────────────────────────────
            framebuffer.drawFastHLine(0, 9, TFT_WIDTH, ST7735_YELLOW);

            // ── Judul halaman ─────────────────────────────────────────
            framebuffer.setTextColor(titleColors[page]);
            framebuffer.setCursor(TFT_LEFT_MARGIN, 11);
            framebuffer.print(pageNames[page]);

            // ── Data baris 1-5 ────────────────────────────────────────
            framebuffer.setTextColor(ST7735_WHITE);
            for (int i = 1; i < TFT_ROWS_PER_PAGE; i++) {
                int y = 11 + i * TFT_LINE_HEIGHT + 2;
                if (y + TFT_LINE_HEIGHT > TFT_HEIGHT) break;
                framebuffer.setCursor(TFT_LEFT_MARGIN, y);
                framebuffer.print(g_pages[page].rows[i]);
            }

            xSemaphoreGive(xTftMutex);

            // SPI transfer dengan mutex
            esp_task_wdt_reset();
            if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(80)) == pdTRUE) {
                tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), TFT_WIDTH, TFT_HEIGHT);
                xSemaphoreGive(xSpiMutex);
            }
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// ============================================
// SD CARD TASK (CPU 0)
// ============================================
#if ENABLE_SD
void sdCardTask(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(1500));
    bool ok = false;
    if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        ok = SD.begin(SD_CS, SPI, 8000000);
        xSemaphoreGive(xSpiMutex);
    }
    if (!ok) { Serial.println("[SD] Init fail"); vTaskDelete(NULL); }
    sdInitialized = true;
    Serial.printf("[SD] OK %luMB\n", (unsigned long)(SD.cardSize()/(1024*1024)));
    if (SD.exists("/tracker.log")) SD.remove("/tracker.log");

    uint32_t lastWrite = 0;
    for (;;) {
        if (millis() - lastWrite >= SD_WRITE_INTERVAL_MS) {
            if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                File f = SD.open("/tracker.log", FILE_APPEND);
                if (f) {
                    char *msg;
                    while (xQueueReceive(xLogQueue, &msg, 0) == pdTRUE) {
                        f.println(msg); delete[] msg;
                    }
                    f.close();
                }
                xSemaphoreGive(xSpiMutex);
            }
            lastWrite = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================
// HELPERS
// ============================================
void logToSd(const char* msg)
{
#if ENABLE_SD
    char *c = new char[strlen(msg)+1];
    strcpy(c, msg);
    if (xQueueSendToBack(xLogQueue, &c, pdMS_TO_TICKS(10)) != pdTRUE) delete[] c;
#endif
}

void saveToSDOffline(float lat, float lng, float alt, uint8_t sat,
                     uint16_t bat, int16_t rssi, float snr, const char* ir)
{
#if ENABLE_SD
    if (!sdInitialized) return;
    if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        File f = SD.open("/offline_queue.csv", FILE_APPEND);
        if (f) {
            char line[200];
            snprintf(line, sizeof(line), "%lu,%.6f,%.6f,%.1f,%u,%u,%d,%.1f,%s",
                     millis(), lat, lng, alt, sat, bat, rssi, snr, ir);
            f.println(line); f.close();
        }
        xSemaphoreGive(xSpiMutex);
    }
#endif
}

bool uploadFromSD()
{
#if ENABLE_WIFI
    if (!sdInitialized) return false;
    bool exists = false;
    if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        exists = SD.exists("/offline_queue.csv");
        xSemaphoreGive(xSpiMutex);
    }
    if (!exists) return false;

    int    okCnt = 0, failCnt = 0;
    String failBuf = "";

    if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        File f = SD.open("/offline_queue.csv", FILE_READ);
        if (!f) { xSemaphoreGive(xSpiMutex); return false; }
        while (f.available()) {
            String line = f.readStringUntil('\n'); line.trim();
            if (line.length() < 10) continue;
            int commas[8], cc = 0;
            for (size_t i = 0; i < line.length() && cc < 8; i++)
                if (line.charAt(i) == ',') commas[cc++] = i;
            if (cc < 7) continue;
            float lat = line.substring(commas[0]+1,commas[1]).toFloat();
            float lng = line.substring(commas[1]+1,commas[2]).toFloat();
            float alt = line.substring(commas[2]+1,commas[3]).toFloat();
            uint8_t sat = line.substring(commas[3]+1,commas[4]).toInt();
            uint16_t bat = line.substring(commas[4]+1,commas[5]).toInt();
            int16_t rssi = line.substring(commas[5]+1,commas[6]).toInt();
            float snr = line.substring(commas[6]+1,commas[7]).toFloat();
            String ir = line.substring(commas[7]+1);

            xSemaphoreGive(xSpiMutex);

            HTTPClient http;
            http.begin(WIFI_API_URL);
            http.addHeader("Content-Type", "application/json");
            http.setTimeout(10000);
            char pl[400];
            snprintf(pl, sizeof(pl),
                "{\"source\":\"wifi_sd\",\"id\":\"%s\","
                "\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.1f,"
                "\"irStatus\":\"%s\",\"battery\":%u,"
                "\"satellites\":%u,\"rssi\":%d,\"snr\":%.1f}",
                DEVICE_ID, lat, lng, alt, ir.c_str(), bat, sat, rssi, snr);
            int code = http.POST(pl);
            http.end();
            if (code == 200 || code == 201) okCnt++;
            else { failCnt++; failBuf += line + "\n"; }

            vTaskDelay(pdMS_TO_TICKS(500));
            xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(200));
        }
        f.close();
        if (failBuf.length() == 0) SD.remove("/offline_queue.csv");
        else { File fw = SD.open("/offline_queue.csv", FILE_WRITE); if(fw){fw.print(failBuf);fw.close();} }
        xSemaphoreGive(xSpiMutex);
    }
    Serial.printf("[SD] upload OK:%d fail:%d\n", okCnt, failCnt);
    return okCnt > 0;
#else
    return false;
#endif
}

String stateDecode(int16_t r)
{
    switch(r) {
        case RADIOLIB_ERR_NONE:               return "OK";
        case RADIOLIB_ERR_CHIP_NOT_FOUND:     return "NO_CHIP";
        case RADIOLIB_ERR_PACKET_TOO_LONG:    return "PKT_LONG";
        case RADIOLIB_ERR_RX_TIMEOUT:         return "RX_TMOUT";
        case RADIOLIB_ERR_MIC_MISMATCH:       return "MIC_ERR";
        case RADIOLIB_ERR_NETWORK_NOT_JOINED: return "NOT_JOIN";
        case RADIOLIB_ERR_NO_JOIN_ACCEPT:     return "NO_ACCEPT";
        case RADIOLIB_LORAWAN_NEW_SESSION:    return "NEW_SESS";
        case RADIOLIB_LORAWAN_SESSION_RESTORED: return "RESTORED";
        default: return "ERR(" + String(r) + ")";
    }
}
