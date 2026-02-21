// ============================================
// HELTEC TRACKER V1.1
// v4 FINAL — TFT SLIDE BERJALAN, Serial minimal
//
// FIX UTAMA:
//   1. HAPUS esp_task_wdt_add() dari tftTask — ini penyebab TFT stuck/kill
//   2. Hapus semua delay() panjang dari setup()
//   3. tftTask dibuat PERTAMA, prioritas tertinggi di CPU 0
//   4. GPRS init di dalam gprsTask (bukan setup)
//   5. Serial Monitor: hanya WiFi/GPRS/SD/LoRa/IR status
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

// PILIH API: true = Local MySQL, false = Vercel
#define USE_LOCAL_MYSQL    true

#if USE_LOCAL_MYSQL
  // Ganti IP_ADDRESS dengan IP komputer Anda
  // Cara cek: CMD → ipconfig → IPv4 Address
  #define WIFI_API_URL     "http://192.168.11.73/lasertag/api/track.php"
#else
  #define WIFI_API_URL     "https://laser-tag-project.vercel.app/api/track"
#endif

// GPRS kirim ke hosting online (bukan lokal - GPRS tidak bisa akses IP privat)
// Format: http://domain:port/folder/api/track.php
#define GPRS_API_URL      "http://simpera.teknoklop.com:10000/lasertag/api/track.php"

#define GPRS_APN           "indosatgprs"
#define GPRS_USER          "indosat"
#define GPRS_PASS          "indosat"

// ThingSpeak (COMMENTED - backup only)
// #define THINGSPEAK_URL     "api.thingspeak.com"
// #define THINGSPEAK_PORT    80
// #define THINGSPEAK_API_KEY "N6ATMD4JVI90HTHH"

#define DEVICE_ID          "Heltec-P1"
#define IR_RECEIVE_PIN     5

// LoRaWAN
const uint64_t joinEUI = 0x0000000000000003;
const uint64_t devEUI  = 0x70B3D57ED0075AC0;
const uint8_t appKey[] = {0x48,0xF0,0x3F,0xDD,0x9C,0x5A,0x9E,0xBA,0x42,0x83,0x01,0x1D,0x7D,0xBB,0xF3,0xF8};
const uint8_t nwkKey[] = {0x9B,0xF6,0x12,0xF4,0xAA,0x33,0xDB,0x73,0xAA,0x1B,0x7A,0xC6,0x4D,0x70,0x02,0x26};

// ============================================
// PIN
// ============================================
#define Vext_Ctrl      3
#define LED_K          21
#define LED_BUILTIN    18
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
const int8_t   TZ_OFFSET          = 7;
const uint32_t UPLINK_INTERVAL_MS = 10000;
const uint32_t WIFI_UPLOAD_MS     = 10000;
const uint32_t GPRS_INTERVAL_MS   = 5000;
const uint8_t  MAX_JOIN_ATTEMPTS  = 5;
const uint32_t JOIN_RETRY_MS      = 2000;
const uint32_t SD_WRITE_MS        = 2000;
const size_t   LOG_QUEUE_SIZE     = 20;
const TickType_t MTX              = pdMS_TO_TICKS(100);
const LoRaWANBand_t Region        = AS923;
const uint8_t subBand             = 0;

// ============================================
// TFT — 3 slide: GPS | LoRa | IR
// ============================================
#define TFT_W          160
#define TFT_H           80
#define TFT_PAGES        3
#define TFT_ROWS         6
#define PAGE_MS       3000
#define LINE_H          10
#define LEFT_M           3

// ============================================
// STRUCTS
// ============================================
struct GpsData    { bool valid,locValid; float lat,lng,alt; uint8_t sat,h,m,s; };
struct LoraStatus { bool joined; String ev; int16_t rssi; float snr; };
struct WifiSt     { bool connected; String ip; unsigned long lastRC; int code; uint32_t ok,fail; };
struct GprsSt     { bool connected; String ip; };
struct IrSt       { bool got; uint16_t addr; uint8_t cmd; unsigned long t; };
struct Page       { String row[TFT_ROWS]; };

struct __attribute__((packed)) Payload {
    uint32_t addr_id, shooter_addr;
    uint8_t  sub_id, shooter_sub, status;
    float    lat,lng,alt;
    uint16_t bat;
    uint8_t  sat;
    int16_t  rssi;
    int8_t   snr;
};

// ============================================
// OBJECTS
// ============================================
TinyGPSPlus GPS;
SX1262      radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 fb(TFT_W, TFT_H);

#if ENABLE_GPRS
HardwareSerial sim900(2);
TinyGsm        modem(sim900);
TinyGsmClient  gsmClient(modem);
#endif

// ============================================
// STATE
// ============================================
GpsData    g_gps  = {};
LoraStatus g_lora = {false,"IDLE",0,0};
WifiSt     g_wifi = {false,"N/A",0,0,0,0};
GprsSt     g_gprs = {false,"N/A"};
IrSt       g_ir   = {};

// 3 slide TFT
Page g_pg[TFT_PAGES] = {
    { {"GPS","Lat:-","Lng:-","Alt:-","Sat:0","--:--:--"} },
    { {"LoRaWAN","Ev:IDLE","RSSI:-","SNR:-","Join:NO","-"} },
    { {"IR","Waiting...","-","-","-","-"} }
};

bool     sdOK   = false;
bool     irSend = false;
bool     irPend = false;
uint16_t irAddr = 0;
uint8_t  irCmd  = 0;

// ============================================
// RTOS
// ============================================
SemaphoreHandle_t mGps,mLora,mTft,mWifi,mGprs,mIr,mSpi;
QueueHandle_t     qLog;

// ============================================
// FORWARD DECLARATIONS
// ============================================
void taskTft(void*);  void taskGps(void*);  void taskLora(void*);
void taskSd(void*);   void taskWifi(void*); void taskGprs(void*); void taskIr(void*);
void logSd(const char*);
void wifiPost(float lat, float lng);
void gprsPost(float lat, float lng);
void sdSave(float,float,float,uint8_t,uint16_t,int16_t,float,const char*);
bool sdUpload();
String decodeState(int16_t);

// ============================================
// SETUP
// Hanya hardware minimal — TIDAK ada delay() panjang
// Semua koneksi jaringan dilakukan di dalam task masing-masing
// ============================================
void setup()
{
    pinMode(Vext_Ctrl,OUTPUT); digitalWrite(Vext_Ctrl,HIGH);
    pinMode(LED_K,OUTPUT);     digitalWrite(LED_K,HIGH);
    pinMode(LED_BUILTIN,OUTPUT); digitalWrite(LED_BUILTIN,LOW);

    Serial.begin(115200);

    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    // TFT splash — TANPA delay panjang
    tft.initR(INITR_MINI160x80);
    tft.setRotation(1);
    tft.fillScreen(ST7735_WHITE);
    tft.setTextColor(ST7735_BLACK); tft.setTextSize(1);
    tft.setCursor(10,15); tft.print("Heltec Tracker");
    tft.setCursor(10,28); tft.print(DEVICE_ID);
    tft.setCursor(10,41); tft.print("Starting...");

    // Mutex & queue
    mGps  = xSemaphoreCreateMutex(); mLora = xSemaphoreCreateMutex();
    mTft  = xSemaphoreCreateMutex(); mWifi = xSemaphoreCreateMutex();
    mGprs = xSemaphoreCreateMutex(); mIr   = xSemaphoreCreateMutex();
    mSpi  = xSemaphoreCreateMutex();
    qLog  = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char*));

    // ── CPU 0: TFT PERTAMA, prioritas 3 (tertinggi) ────────────────
    // INI KUNCI: TFT harus jalan duluan sebelum task lain
    xTaskCreatePinnedToCore(taskTft, "TFT", 32768, NULL, 3, NULL, 0);

    // CPU 0: SD (prioritas rendah, ada delay internal 3 detik)
#if ENABLE_SD
    xTaskCreatePinnedToCore(taskSd,  "SD",   8192, NULL, 1, NULL, 0);
#endif

    // ── CPU 1: sensor & jaringan ────────────────────────────────────
    xTaskCreatePinnedToCore(taskGps,  "GPS",  8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(taskIr,   "IR",   4096, NULL, 2, NULL, 1);
#if ENABLE_WIFI
    xTaskCreatePinnedToCore(taskWifi, "WiFi",32768, NULL, 1, NULL, 1);
#endif
#if ENABLE_LORAWAN
    xTaskCreatePinnedToCore(taskLora, "LoRa",16384, NULL, 1, NULL, 1);
#endif
#if ENABLE_GPRS
    xTaskCreatePinnedToCore(taskGprs, "GPRS",32768, NULL, 1, NULL, 1);
#endif
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }

// ============================================
// TFT TASK (CPU 0, prioritas 3)
//
// PENYEBAB TFT STUCK SEBELUMNYA:
//   - esp_task_wdt_add(NULL) tanpa config WDT timeout = watchdog
//     langsung reset/kill task karena timeout default sangat pendek
//   - setup() punya delay(1500) + delay(1000) GPRS sebelum task dibuat
//     sehingga scheduler belum jalan
//
// FIX:
//   - TIDAK pakai esp_task_wdt_add sama sekali
//   - vTaskDelay(2000) sekali di awal = splash 2 detik
//   - Loop 100ms = render ~10fps, slide tiap PAGE_MS (3 detik)
//   - mSpi melindungi SPI dari konflik dengan SD
// ============================================
void taskTft(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(2000)); // Splash 2 detik

    const char*    name[TFT_PAGES]  = {"GPS", "LoRaWAN", "IR"};
    // Warna judul gelap agar kontras di latar putih
    // GPS=hijau tua, LoRa=oranye, IR=ungu tua
    const uint16_t color[TFT_PAGES] = {0x03A0, 0xC800, 0x8010};

    uint32_t tSwitch = millis();
    uint8_t  pg      = 0;

    for(;;)
    {
        // Auto-slide setiap PAGE_MS
        if(millis() - tSwitch >= PAGE_MS){
            pg = (pg + 1) % TFT_PAGES;
            tSwitch = millis();
        }

        // Snapshot data halaman saat ini
        Page snap;
        if(xSemaphoreTake(mTft, pdMS_TO_TICKS(15)) == pdTRUE){
            snap = g_pg[pg];
            xSemaphoreGive(mTft);
        }

        // Baca status koneksi
        bool wOk=false, lOk=false, gOk=false, sOk=sdOK;
        if(xSemaphoreTake(mWifi, pdMS_TO_TICKS(5)) == pdTRUE){ wOk=g_wifi.connected; xSemaphoreGive(mWifi); }
        if(xSemaphoreTake(mLora, pdMS_TO_TICKS(5)) == pdTRUE){ lOk=g_lora.joined;    xSemaphoreGive(mLora); }
        if(xSemaphoreTake(mGprs, pdMS_TO_TICKS(5)) == pdTRUE){ gOk=g_gprs.connected; xSemaphoreGive(mGprs); }

        // Render ke framebuffer
        fb.fillScreen(ST7735_WHITE);
        fb.setTextSize(1);

        // Status bar atas: W:OK L:OK G:OK S:OK  [n/3]
        fb.setCursor(0,0);
        fb.setTextColor(wOk?ST7735_GREEN:ST7735_RED); fb.print(wOk?"W:OK ":"W:X  ");
        fb.setTextColor(lOk?ST7735_GREEN:ST7735_RED); fb.print(lOk?"L:OK ":"L:X  ");
        fb.setTextColor(gOk?ST7735_GREEN:ST7735_RED); fb.print(gOk?"G:OK ":"G:X  ");
        fb.setTextColor(sOk?ST7735_GREEN:ST7735_RED); fb.print(sOk?"S:OK":"S:X ");
        fb.setTextColor(ST7735_BLACK);
        fb.setCursor(TFT_W - 18, 0); fb.printf("%d/3", pg+1);

        // Garis pemisah
        fb.drawFastHLine(0, 9, TFT_W, ST7735_BLACK);

        // Judul slide
        fb.setTextColor(color[pg]);
        fb.setCursor(LEFT_M, 11); fb.print(name[pg]);

        // Data baris 1–5
        fb.setTextColor(ST7735_BLACK);
        for(int i = 1; i < TFT_ROWS; i++){
            int y = 11 + i * LINE_H + 2;
            if(y + LINE_H > TFT_H) break;
            fb.setCursor(LEFT_M, y);
            fb.print(snap.row[i]);
        }

        // Flush ke layar (SPI protected)
        if(xSemaphoreTake(mSpi, pdMS_TO_TICKS(40)) == pdTRUE){
            tft.drawRGBBitmap(0, 0, fb.getBuffer(), TFT_W, TFT_H);
            xSemaphoreGive(mSpi);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// GPS TASK (CPU 1)
// ============================================
void taskGps(void *pv)
{
    for(;;){
        int n = Serial1.available();
        for(int i = 0; i < n; i++) GPS.encode(Serial1.read());

        bool hasT = GPS.time.isValid(), hasL = GPS.location.isValid();

        if(xSemaphoreTake(mGps, MTX) == pdTRUE){
            g_gps.valid    = hasT || hasL;
            g_gps.locValid = hasL;
            g_gps.sat      = GPS.satellites.value();
            if(hasL){ g_gps.lat=GPS.location.lat(); g_gps.lng=GPS.location.lng(); g_gps.alt=GPS.altitude.meters(); }
            if(hasT){ g_gps.h=GPS.time.hour(); g_gps.m=GPS.time.minute(); g_gps.s=GPS.time.second(); }
            xSemaphoreGive(mGps);
        }

        if(xSemaphoreTake(mTft, MTX) == pdTRUE){
            int8_t dh = 0;
            if(hasT){ dh=(int8_t)(g_gps.h+TZ_OFFSET); if(dh>=24)dh-=24; if(dh<0)dh+=24; }
            char tb[12]; snprintf(tb,12,"%02d:%02d:%02d", hasT?dh:0, hasT?g_gps.m:0, hasT?g_gps.s:0);
            g_pg[0].row[0] = "GPS";
            g_pg[0].row[1] = hasL ? ("Lat:"+String(g_gps.lat,5)) : "Lat:-";
            g_pg[0].row[2] = hasL ? ("Lng:"+String(g_gps.lng,5)) : "Lng:-";
            g_pg[0].row[3] = hasL ? ("Alt:"+String(g_gps.alt,1)+"m") : "Alt:-";
            g_pg[0].row[4] = "Sat:"+String(g_gps.sat);
            g_pg[0].row[5] = String(tb);
            xSemaphoreGive(mTft);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================
// LORAWAN TASK (CPU 1)
// Serial: "[LoRa] Connected" atau "[LoRa] No join"
// ============================================
#if ENABLE_LORAWAN
void taskLora(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    delay(100);

    auto setPage = [](const char* ev, int16_t rssi, float snr, bool joined){
        if(xSemaphoreTake(mTft, pdMS_TO_TICKS(30)) == pdTRUE){
            g_pg[1].row[0] = "LoRaWAN";
            g_pg[1].row[1] = String("Ev:") + ev;
            g_pg[1].row[2] = "RSSI:"+String(rssi)+"dBm";
            g_pg[1].row[3] = "SNR:"+String(snr,1)+"dB";
            g_pg[1].row[4] = joined ? "Join:YES" : "Join:NO";
            g_pg[1].row[5] = "-";
            xSemaphoreGive(mTft);
        }
    };

    if(radio.begin() != RADIOLIB_ERR_NONE){ setPage("FAIL",0,0,false); vTaskDelete(NULL); }
    if(node.beginOTAA(joinEUI,devEUI,nwkKey,appKey) != RADIOLIB_ERR_NONE){ setPage("INIT_FAIL",0,0,false); vTaskDelete(NULL); }

    for(uint8_t a = 1; a <= MAX_JOIN_ATTEMPTS; a++){
        char jm[14]; snprintf(jm,14,"JOIN%d/%d",a,MAX_JOIN_ATTEMPTS);
        setPage(jm, 0, 0, false);
        int16_t s = node.activateOTAA();
        vTaskDelay(pdMS_TO_TICKS(10));
        if(s == RADIOLIB_LORAWAN_NEW_SESSION){
            Serial.println("[LoRa] Connected");
            if(xSemaphoreTake(mLora,MTX)==pdTRUE){ g_lora.joined=true; xSemaphoreGive(mLora); }
            setPage("JOINED", 0, 0, true);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(JOIN_RETRY_MS));
    }
    if(!g_lora.joined){ Serial.println("[LoRa] No join"); setPage("NO_JOIN",0,0,false); vTaskDelete(NULL); }

    uint32_t tUp = 0;
    for(;;){
        if(irSend || millis()-tUp >= UPLINK_INTERVAL_MS){
            irSend = false;
            Payload pl = {};
            if(xSemaphoreTake(mGps,MTX)==pdTRUE){ pl.sat=g_gps.sat; pl.lat=g_gps.lat; pl.lng=g_gps.lng; pl.alt=g_gps.alt; xSemaphoreGive(mGps); }
            if(irPend){ pl.shooter_sub=1; pl.shooter_addr=irAddr; pl.sub_id=irCmd; pl.status=1; irPend=false; }
            else if(xSemaphoreTake(mIr,MTX)==pdTRUE){
                pl.shooter_addr=g_ir.got?g_ir.addr:0; pl.sub_id=g_ir.got?g_ir.cmd:0;
                pl.shooter_sub=g_ir.got?1:0; pl.status=g_ir.got?1:0;
                xSemaphoreGive(mIr);
            }
            if(xSemaphoreTake(mLora,MTX)==pdTRUE){ pl.rssi=g_lora.rssi; pl.snr=(int8_t)(g_lora.snr+0.5f); xSemaphoreGive(mLora); }
            uint8_t buf[sizeof(Payload)]; memcpy(buf,&pl,sizeof(pl));
            int16_t s = node.sendReceive(buf, sizeof(buf));
            String ev; float snr=0; int16_t rssi=0;
            if(s < RADIOLIB_ERR_NONE){ ev="TX_FAIL"; }
            else{
                ev = s>0 ? "TX+RX" : "TX_OK";
                snr=radio.getSNR(); rssi=radio.getRSSI();
                if(xSemaphoreTake(mIr,MTX)==pdTRUE){ g_ir.got=false; xSemaphoreGive(mIr); }
            }
            if(xSemaphoreTake(mLora,MTX)==pdTRUE){ g_lora.ev=ev; g_lora.snr=snr; g_lora.rssi=rssi; xSemaphoreGive(mLora); }
            setPage(ev.c_str(), rssi, snr, true);
            tUp = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================
// WIFI TASK (CPU 1)
// Serial: "[WiFi] Connected IP: x.x.x.x"
// ============================================
#if ENABLE_WIFI
void taskWifi(void *pv)
{
    WiFi.disconnect(true); vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long t0 = millis();
    while(WiFi.status() != WL_CONNECTED && millis()-t0 < 20000)
        vTaskDelay(pdMS_TO_TICKS(500));

    if(WiFi.status() == WL_CONNECTED){
        String ip = WiFi.localIP().toString();
        Serial.printf("[WiFi] Connected IP: %s\n", ip.c_str());
        if(xSemaphoreTake(mWifi,MTX)==pdTRUE){ g_wifi.connected=true; g_wifi.ip=ip; xSemaphoreGive(mWifi); }
        sdUpload();
    } else {
        Serial.println("[WiFi] Connect failed");
    }

    uint32_t tUp = 0;
    for(;;){
        if(WiFi.status() != WL_CONNECTED){
            if(xSemaphoreTake(mWifi,MTX)==pdTRUE){ g_wifi.connected=false; xSemaphoreGive(mWifi); }
            if(millis()-g_wifi.lastRC >= 10000){
                g_wifi.lastRC = millis();
                WiFi.disconnect(true); vTaskDelay(pdMS_TO_TICKS(300));
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                unsigned long tw = millis();
                while(WiFi.status()!=WL_CONNECTED && millis()-tw<15000) vTaskDelay(pdMS_TO_TICKS(500));
                if(WiFi.status() == WL_CONNECTED){
                    String ip = WiFi.localIP().toString();
                    Serial.printf("[WiFi] Reconnected IP: %s\n", ip.c_str());
                    if(xSemaphoreTake(mWifi,MTX)==pdTRUE){ g_wifi.connected=true; g_wifi.ip=ip; xSemaphoreGive(mWifi); }
                }
            }
        } else {
            if(xSemaphoreTake(mWifi,MTX)==pdTRUE){
                if(!g_wifi.connected){ g_wifi.connected=true; g_wifi.ip=WiFi.localIP().toString(); }
                xSemaphoreGive(mWifi);
            }
        }

        if(g_wifi.connected && millis()-tUp >= WIFI_UPLOAD_MS){
            float lat=0,lng=0;
            if(xSemaphoreTake(mGps,MTX)==pdTRUE){ lat=g_gps.lat; lng=g_gps.lng; xSemaphoreGive(mGps); }
            wifiPost(lat, lng);
            tUp = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void wifiPost(float lat, float lng)
{
    char irBuf[32]="-"; float alt=0; uint8_t sat=0; int16_t rssi=0; float snr=0;
    uint16_t ia=0; uint8_t ic=0;
    const char* hitStatus = "NONE";
    if(xSemaphoreTake(mIr,MTX)==pdTRUE){
        if(g_ir.got){ 
            snprintf(irBuf,32,"HIT:0x%X-0x%X",g_ir.addr,g_ir.cmd); 
            ia=g_ir.addr; 
            ic=g_ir.cmd;
            hitStatus = "HIT";
        }
        xSemaphoreGive(mIr);
    }
    if(xSemaphoreTake(mGps,MTX)==pdTRUE){ alt=g_gps.alt; sat=g_gps.sat; xSemaphoreGive(mGps); }
    if(xSemaphoreTake(mLora,MTX)==pdTRUE){ rssi=g_lora.rssi; snr=g_lora.snr; xSemaphoreGive(mLora); }

    char pl[400];
    snprintf(pl, sizeof(pl),
        "{\"deviceId\":\"%s\",\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.1f,"
        "\"sats\":%u,\"rssi\":%d,\"snr\":%.1f,\"battery\":0,"
        "\"irAddress\":%u,\"irCommand\":%u,\"hitStatus\":\"%s\"}",
        DEVICE_ID, lat, lng, alt, sat, rssi, snr, ia, ic, hitStatus);

    int code=0; bool ok=false;
    for(int i=1; i<=3 && !ok; i++){
        if(WiFi.status() != WL_CONNECTED) break;
        HTTPClient http; http.begin(WIFI_API_URL);
        http.addHeader("Content-Type","application/json"); http.setTimeout(10000);
        code = http.POST(pl); ok = (code==200||code==201); http.end();
        if(!ok && i<3) vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if(xSemaphoreTake(mWifi,MTX)==pdTRUE){ g_wifi.code=code; if(ok)g_wifi.ok++;else g_wifi.fail++; xSemaphoreGive(mWifi); }
    if(!ok) sdSave(lat,lng,alt,sat,0,rssi,snr,irBuf);
}
#endif

// ============================================
// GPRS TASK (CPU 1)
// Init hardware SIM900A di dalam task, bukan setup()
// Serial: "[GPRS] Connected IP: x.x.x.x"
// ============================================
#if ENABLE_GPRS
void taskGprs(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(8000)); // Tunggu WiFi + LoRa

    // Init hardware SIM900A
    pinMode(GPRS_RST_PIN,OUTPUT);
    digitalWrite(GPRS_RST_PIN,HIGH); vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(GPRS_RST_PIN,LOW);  vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(GPRS_RST_PIN,HIGH); vTaskDelay(pdMS_TO_TICKS(1000));
    sim900.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
    vTaskDelay(pdMS_TO_TICKS(1000));

    if(!modem.begin()){ Serial.println("[GPRS] Modem fail"); vTaskDelete(NULL); }

    bool conn = false;
    for(int r=0; r<5&&!conn; r++){
        if(modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)){
            conn = true;
            String ip = modem.getLocalIP();
            Serial.printf("[GPRS] Connected IP: %s\n", ip.c_str());
            if(xSemaphoreTake(mGprs,MTX)==pdTRUE){ g_gprs.connected=true; g_gprs.ip=ip; xSemaphoreGive(mGprs); }
        } else vTaskDelay(pdMS_TO_TICKS(5000));
    }
    if(!conn){ Serial.println("[GPRS] Failed"); vTaskDelete(NULL); }

    uint32_t tSend = 0;
    for(;;){
        if(!modem.isGprsConnected()){
            if(modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)){
                String ip = modem.getLocalIP();
                Serial.printf("[GPRS] Reconnected IP: %s\n", ip.c_str());
                if(xSemaphoreTake(mGprs,MTX)==pdTRUE){ g_gprs.connected=true; g_gprs.ip=ip; xSemaphoreGive(mGprs); }
            } else {
                if(xSemaphoreTake(mGprs,MTX)==pdTRUE){ g_gprs.connected=false; xSemaphoreGive(mGprs); }
                vTaskDelay(pdMS_TO_TICKS(5000)); continue;
            }
        }
        if(millis()-tSend >= GPRS_INTERVAL_MS){
            float lat=0,lng=0; bool locOk=false;
            if(xSemaphoreTake(mGps,MTX)==pdTRUE){ lat=g_gps.lat; lng=g_gps.lng; locOk=g_gps.locValid; xSemaphoreGive(mGps); }
            if(locOk) gprsPost(lat, lng);
            tSend = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void gprsPost(float lat, float lng)
{
    char irBuf[32]="-"; float alt=0; uint8_t sat=0; int16_t rssi=0; float snr=0;
    uint16_t ia=0; uint8_t ic=0;
    const char* hitStatus = "NONE";
    if(xSemaphoreTake(mIr,MTX)==pdTRUE){ 
        if(g_ir.got){ 
            snprintf(irBuf,32,"HIT:0x%X-0x%X",g_ir.addr,g_ir.cmd); 
            ia=g_ir.addr; 
            ic=g_ir.cmd;
            hitStatus = "HIT";
        }
        xSemaphoreGive(mIr); 
    }
    if(xSemaphoreTake(mGps,MTX)==pdTRUE){ alt=g_gps.alt; sat=g_gps.sat; xSemaphoreGive(mGps); }
    if(xSemaphoreTake(mLora,MTX)==pdTRUE){ rssi=g_lora.rssi; snr=g_lora.snr; xSemaphoreGive(mLora); }

    char pl[400];
    snprintf(pl, sizeof(pl),
        "{\"deviceId\":\"%s\",\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.1f,"
        "\"sats\":%u,\"rssi\":%d,\"snr\":%.1f,"
        "\"irAddress\":%u,\"irCommand\":%u,\"hitStatus\":\"%s\"}",
        DEVICE_ID, lat, lng, alt, sat, rssi, snr, ia, ic, hitStatus);

    // Kirim ke Hosting via GPRS (HTTP port 10000)
    const char* host = "simpera.teknoklop.com";
    const char* path = "/lasertag/api/track.php";
    int port = 10000;
    
    if(gsmClient.connect(host, port)){
        gsmClient.printf("POST %s HTTP/1.1\r\n", path);
        gsmClient.printf("Host: %s:%d\r\n", host, port);
        gsmClient.printf("Content-Type: application/json\r\n");
        gsmClient.printf("Content-Length: %d\r\n", strlen(pl));
        gsmClient.printf("Connection: close\r\n");
        gsmClient.printf("\r\n");
        gsmClient.print(pl);
        
        unsigned long t=millis();
        while(!gsmClient.available() && millis()-t<15000) vTaskDelay(pdMS_TO_TICKS(10));
        while(gsmClient.available()){ gsmClient.read(); vTaskDelay(1); }
        gsmClient.stop();
        Serial.println("[GPRS] Data sent to Hosting");
    } else {
        Serial.println("[GPRS] Failed to connect");
    }
}
#endif

// ============================================
// IR TASK (CPU 1)
// Serial: "[IR] Addr: 0x....  Command: 0x.."
// ============================================
void taskIr(void *pv)
{
    for(;;){
        if(IrReceiver.decode()){
            if(IrReceiver.decodedIRData.protocol == NEC){
                uint16_t a = IrReceiver.decodedIRData.address;
                uint8_t  c = IrReceiver.decodedIRData.command;
                if(xSemaphoreTake(mIr,MTX)==pdTRUE){
                    g_ir.got=true; g_ir.addr=a; g_ir.cmd=c; g_ir.t=millis();
                    irPend=true; irAddr=a; irCmd=c; irSend=true;
                    xSemaphoreGive(mIr);
                }
                Serial.printf("[IR] Addr: 0x%04X  Command: 0x%02X\n", a, c);
                char buf[32]; snprintf(buf,32,"IR:0x%04X,0x%02X",a,c); logSd(buf);
            }
            IrReceiver.resume();
        }

        if(xSemaphoreTake(mTft,MTX)==pdTRUE){
            if(xSemaphoreTake(mIr,MTX)==pdTRUE){
                g_pg[2].row[0] = "IR Receiver";
                if(g_ir.got){
                    g_pg[2].row[1] = "Proto:NEC";
                    g_pg[2].row[2] = "Addr:0x" + String(g_ir.addr, HEX);
                    g_pg[2].row[3] = "Cmd: 0x" + String(g_ir.cmd, HEX);
                    g_pg[2].row[4] = "+" + String((millis()-g_ir.t)/1000) + "s ago";
                    g_pg[2].row[5] = "-";
                } else {
                    g_pg[2].row[1]="Waiting...";
                    g_pg[2].row[2]=g_pg[2].row[3]=g_pg[2].row[4]=g_pg[2].row[5]="-";
                }
                xSemaphoreGive(mIr);
            }
            xSemaphoreGive(mTft);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// SD TASK (CPU 0)
// Serial: "[SD] xxMB connected"
// Delay 3 detik internal agar TFT tidak rebut SPI saat splash
// ============================================
#if ENABLE_SD
void taskSd(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    bool ok = false;
    if(xSemaphoreTake(mSpi, pdMS_TO_TICKS(500))==pdTRUE){
        ok = SD.begin(SD_CS, SPI, 8000000);
        xSemaphoreGive(mSpi);
    }
    if(!ok){ Serial.println("[SD] Init fail"); vTaskDelete(NULL); }
    sdOK = true;
    Serial.printf("[SD] %luMB connected\n", (unsigned long)(SD.cardSize()/(1024*1024)));
    if(SD.exists("/tracker.log")) SD.remove("/tracker.log");

    uint32_t tW = 0;
    for(;;){
        if(millis()-tW >= SD_WRITE_MS){
            if(xSemaphoreTake(mSpi, pdMS_TO_TICKS(100))==pdTRUE){
                File f = SD.open("/tracker.log", FILE_APPEND);
                if(f){ char*msg; while(xQueueReceive(qLog,&msg,0)==pdTRUE){f.println(msg);delete[]msg;} f.close(); }
                xSemaphoreGive(mSpi);
            }
            tW = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

// ============================================
// HELPERS
// ============================================
void logSd(const char*msg){
#if ENABLE_SD
    char*c=new char[strlen(msg)+1]; strcpy(c,msg);
    if(xQueueSendToBack(qLog,&c,pdMS_TO_TICKS(10))!=pdTRUE) delete[]c;
#endif
}

void sdSave(float lat,float lng,float alt,uint8_t sat,uint16_t bat,int16_t rssi,float snr,const char*ir){
#if ENABLE_SD
    if(!sdOK) return;
    if(xSemaphoreTake(mSpi,pdMS_TO_TICKS(100))==pdTRUE){
        File f=SD.open("/offline_queue.csv",FILE_APPEND);
        if(f){ char ln[200]; snprintf(ln,200,"%lu,%.6f,%.6f,%.1f,%u,%u,%d,%.1f,%s",millis(),lat,lng,alt,sat,bat,rssi,snr,ir); f.println(ln); f.close(); }
        xSemaphoreGive(mSpi);
    }
#endif
}

bool sdUpload(){
#if ENABLE_WIFI
    if(!sdOK) return false;
    bool ex=false;
    if(xSemaphoreTake(mSpi,pdMS_TO_TICKS(200))==pdTRUE){ ex=SD.exists("/offline_queue.csv"); xSemaphoreGive(mSpi); }
    if(!ex) return false;
    int okCnt=0; String fail="";
    if(xSemaphoreTake(mSpi,pdMS_TO_TICKS(200))==pdTRUE){
        File f=SD.open("/offline_queue.csv",FILE_READ);
        if(!f){xSemaphoreGive(mSpi);return false;}
        while(f.available()){
            String ln=f.readStringUntil('\n'); ln.trim(); if(ln.length()<10) continue;
            int c[8],cc=0; for(size_t i=0;i<ln.length()&&cc<8;i++) if(ln.charAt(i)==',') c[cc++]=i;
            if(cc<7) continue;
            float lat=ln.substring(c[0]+1,c[1]).toFloat(), lng=ln.substring(c[1]+1,c[2]).toFloat(), alt=ln.substring(c[2]+1,c[3]).toFloat();
            uint8_t sat=ln.substring(c[3]+1,c[4]).toInt(); uint16_t bat=ln.substring(c[4]+1,c[5]).toInt();
            int16_t rssi=ln.substring(c[5]+1,c[6]).toInt(); float snr=ln.substring(c[6]+1,c[7]).toFloat();
            String ir=ln.substring(c[7]+1);
            xSemaphoreGive(mSpi);
            HTTPClient http; http.begin(WIFI_API_URL); http.addHeader("Content-Type","application/json"); http.setTimeout(10000);
            char pl[400]; snprintf(pl,400,"{\"source\":\"wifi_sd\",\"id\":\"%s\",\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.1f,\"irStatus\":\"%s\",\"battery\":%u,\"satellites\":%u,\"rssi\":%d,\"snr\":%.1f}",DEVICE_ID,lat,lng,alt,ir.c_str(),bat,sat,rssi,snr);
            int code=http.POST(pl); http.end();
            if(code==200||code==201) okCnt++; else fail+=ln+"\n";
            vTaskDelay(pdMS_TO_TICKS(500));
            xSemaphoreTake(mSpi,pdMS_TO_TICKS(200));
        }
        f.close();
        if(fail.length()==0) SD.remove("/offline_queue.csv");
        else{ File fw=SD.open("/offline_queue.csv",FILE_WRITE); if(fw){fw.print(fail);fw.close();} }
        xSemaphoreGive(mSpi);
    }
    return okCnt>0;
#else
    return false;
#endif
}

String decodeState(int16_t r){
    switch(r){
        case RADIOLIB_ERR_NONE:                 return "OK";
        case RADIOLIB_ERR_CHIP_NOT_FOUND:       return "NO_CHIP";
        case RADIOLIB_ERR_NETWORK_NOT_JOINED:   return "NOT_JOIN";
        case RADIOLIB_ERR_NO_JOIN_ACCEPT:       return "NO_ACCEPT";
        case RADIOLIB_LORAWAN_NEW_SESSION:      return "NEW_SESS";
        case RADIOLIB_LORAWAN_SESSION_RESTORED: return "RESTORED";
        default: return "ERR("+String(r)+")";
    }
}
