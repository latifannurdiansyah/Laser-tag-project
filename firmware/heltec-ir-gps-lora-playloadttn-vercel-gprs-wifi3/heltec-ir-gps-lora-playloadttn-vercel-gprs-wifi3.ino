// ============================================
// HELTEC TRACKER V1.1
// Modifikasi: TFT 4 halaman (hitam), GPRS task update + Cheat Detection
// Update: Status Bar Atas (L:OK G:OK W:OK S:OK [n/4])
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
#define USE_DUAL_MODE      true
#define LOCAL_API_URL      "http://192.168.11.96/lasertag/api/track.php"
#define VERCEL_API_URL     "https://laser-tag-project.vercel.app/api/track"
#define GPRS_API_URL       "http://simpera.teknoklop.com:10000/lasertag/api/track.php"
#define GPRS_CHEAT_URL     "http://simpera.teknoklop.com:10000/lasertag/api/cheat_event.php"
#define GPRS_APN           "indosatgprs"
#define GPRS_USER          "indosat"
#define GPRS_PASS          "indosat"
#define GPRS_SIM_PIN       ""
#define GPRS_HOST          "simpera.teknoklop.com"
#define GPRS_PORT          10000
#define GPRS_API_KEY       "tPmAT5Ab3j7F9"
#define GPRS_PATH          "/lasertag/api/track.php"
#define GPRS_CHEAT_PATH    "/lasertag/api/cheat_event.php"
#define DEVICE_ID          "Heltec-P1"
#define IR_RECEIVE_PIN     5
// ============================================
// ANTI-CHEAT CONFIG
// ============================================
#define IR_AMBIENT_PIN        2
#define CHEAT_DETECTION_MS    5000
#define CHEAT_CHECK_INTERVAL  100
#define CHEAT_ALERT_DURATION  10000
#define IR_IMMEDIATE_SEND     2
// ============================================
// LORAWAN KEYS
// ============================================
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
const uint32_t UPLINK_INTERVAL_MS = 1000;
const uint32_t WIFI_UPLOAD_MS     = 1000;
const uint32_t GPRS_INTERVAL_MS   = 30000;
const uint8_t  MAX_JOIN_ATTEMPTS  = 5;
const uint32_t JOIN_RETRY_MS      = 2000;
const uint32_t SD_WRITE_MS        = 2000;
const size_t   LOG_QUEUE_SIZE     = 20;
const TickType_t MTX              = pdMS_TO_TICKS(100);
const LoRaWANBand_t Region        = AS923;
const uint8_t subBand             = 0;
// ============================================
// TFT — 4 halaman: LoRaWAN | GPS | GPRS | IR
// ============================================
#define TFT_W      160
#define TFT_H       80
#define TFT_PAGES    4
#define TFT_ROWS     6
#define PAGE_MS   3000
#define LINE_H      10
#define LEFT_M       3
#define TOP_M       10
// ============================================
// STRUCTS
// ============================================
struct GpsData    { bool valid, locValid; float lat, lng, alt; uint8_t sat, h, m, s; };
struct LoraStatus { bool joined; String ev; int16_t rssi; float snr; };
struct WifiSt     { bool connected; String ip; unsigned long lastRC; int code; uint32_t ok, fail; };
struct GprsSt     { bool connected; String ip; String lastEvt; String lastResp; };
struct IrSt       { bool got; uint16_t addr; uint8_t cmd; unsigned long t; uint32_t hitCount; };
struct Page       { String row[TFT_ROWS]; };
struct __attribute__((packed)) Payload {
  uint32_t addr_id, shooter_addr;
  uint8_t  sub_id, shooter_sub, status;
  float    lat, lng, alt;
  uint16_t bat;
  uint8_t  sat;
  int16_t  rssi;
  int8_t   snr;
  uint8_t  cheatDetected;
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
LoraStatus g_lora = {false, "IDLE", 0, 0};
WifiSt     g_wifi = {false, "N/A", 0, 0, 0, 0};
GprsSt     g_gprs = {false, "N/A", "IDLE", "-"};
IrSt       g_ir   = {};
volatile bool     g_cheatDetected   = false;
volatile uint32_t g_cheatAlertExpiry = 0;
Page g_pg[TFT_PAGES] = {
  { { "LoRaWAN Status ",  "Event: IDLE ",  "RSSI: -",  "SNR: -",  "Joined: NO ",  "-"} },
  { { "GPS Status ",  "Lat: -",  "Long: -",  "Alt: -",  "Sat: -",  "--:--:--"} },
  { { "GPRS/DB Status ",  "Conn: NO ",  "IP: -",  "Last: -",  "Resp: -",  "Evt: IDLE "} },
  { { "IR Hit Info ",  "Shooter: -",  "Sub: -",  "Loc: NONE ",  "Time: --:--",  "Hits: 0 "} }
};
bool     sdOK   = false;
bool     irSend = false;
bool     irPend = false;
uint16_t irAddr = 0;
uint8_t  irCmd  = 0;
// ============================================
// RTOS
// ============================================
SemaphoreHandle_t mGps, mLora, mTft, mWifi, mGprs, mIr, mSpi;
QueueHandle_t     qLog;
// ============================================
// FORWARD DECLARATIONS
// ============================================
void taskTft(void*);
void taskGps(void*);
void taskLora(void*);
void taskSd(void*);
void taskWifi(void*);
void taskGprsLoop(void*);
void taskIr(void*);
void taskAntiCheat(void*);
void logSd(const char*);
void wifiPost(float lat, float lng);
void sendImmediateGprsPayload(bool isCheat);
void buildDataPayload(Payload &payload);
void sdSave(float, float, float, uint8_t, uint16_t, int16_t, float, const char*);
bool sdUpload();
String decodeState(int16_t);
void initGprsHardware();
// ============================================
// SETUP
// ============================================
void setup()
{
  pinMode(Vext_Ctrl, OUTPUT); digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);     digitalWrite(LED_K, HIGH);
  pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, LOW);
  pinMode(IR_AMBIENT_PIN, INPUT);

  Serial.begin(115200);
  Serial.println("\n[BOOT] Heltec Tracker V1.1 (LoRa+WiFi+GPRS+IR+AntiCheat)");

  Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_CYAN); tft.setTextSize(1);
  tft.setCursor(10, 15); tft.print("Heltec Tracker");
  tft.setCursor(10, 28); tft.print(DEVICE_ID);
  tft.setCursor(10, 41); tft.setTextColor(ST7735_WHITE); tft.print("Starting...");

  mGps  = xSemaphoreCreateMutex();
  mLora = xSemaphoreCreateMutex();
  mTft  = xSemaphoreCreateMutex();
  mWifi = xSemaphoreCreateMutex();
  mGprs = xSemaphoreCreateMutex();
  mIr   = xSemaphoreCreateMutex();
  mSpi  = xSemaphoreCreateMutex();
  qLog  = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char*));

  xTaskCreatePinnedToCore(taskTft, "TFT", 32768, NULL, 3, NULL, 0);

#if ENABLE_SD
  xTaskCreatePinnedToCore(taskSd, "SD", 8192, NULL, 1, NULL, 0);
#endif
  xTaskCreatePinnedToCore(taskGps,        "GPS",      8192,  NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskIr,         "IR",       4096,  NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskAntiCheat,  "Cheat",    2048,  NULL, 2, NULL, 1);
#if ENABLE_WIFI
  xTaskCreatePinnedToCore(taskWifi, "WiFi", 32768, NULL, 1, NULL, 1);
#endif
#if ENABLE_LORAWAN
  xTaskCreatePinnedToCore(taskLora, "LoRa", 16384, NULL, 1, NULL, 1);
#endif
#if ENABLE_GPRS
  initGprsHardware();
  xTaskCreatePinnedToCore(taskGprsLoop, "GPRS", 32768, NULL, 1, NULL, 1);
#endif
}
void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
// ============================================
// TFT TASK
// ============================================
void taskTft(void *pv)
{
  vTaskDelay(pdMS_TO_TICKS(2000));
  uint32_t pageSwitch = millis();
  uint8_t  currentPage = 0;

  for (;;)
  {
    if (millis() - pageSwitch >= PAGE_MS) {
      currentPage = (currentPage + 1) % TFT_PAGES;
      pageSwitch = millis();
    }

    if (xSemaphoreTake(mTft, pdMS_TO_TICKS(100)) == pdTRUE) {

      fb.fillScreen(ST7735_BLACK);
      fb.setTextColor(ST7735_WHITE);
      fb.setTextSize(1);

      // STATUS BAR ATAS
      fb.setCursor(0, 1);
      
      fb.print("L:");
      fb.setTextColor(g_lora.joined ? ST7735_GREEN : ST7735_RED);
      fb.print(g_lora.joined ? "OK" : "NO");
      fb.setTextColor(ST7735_WHITE);
      fb.print(" ");

      fb.print("G:");
      fb.setTextColor(g_gprs.connected ? ST7735_GREEN : ST7735_RED);
      fb.print(g_gprs.connected ? "OK" : "NO");
      fb.setTextColor(ST7735_WHITE);
      fb.print(" ");

      fb.print("W:");
      fb.setTextColor(g_wifi.connected ? ST7735_GREEN : ST7735_RED);
      fb.print(g_wifi.connected ? "OK" : "NO");
      fb.setTextColor(ST7735_WHITE);
      fb.print(" ");

      fb.print("S:");
      fb.setTextColor(sdOK ? ST7735_GREEN : ST7735_RED);
      fb.print(sdOK ? "OK" : "NO");
      fb.setTextColor(ST7735_WHITE);
      fb.print(" ");

      fb.printf("[%d/%d]", currentPage + 1, TFT_PAGES);

      // Garis Pembatas (ST7735_BLUE menggantikan ST7735_GRAY)
      fb.drawLine(0, 9, TFT_W, 9, ST7735_BLUE);

      // Render 6 baris halaman aktif
      for (int i = 0; i < TFT_ROWS; i++) {
        int y = TOP_M + i * LINE_H;
        if (y >= TFT_H) break;
        fb.setCursor(LEFT_M, y);
        fb.print(g_pg[currentPage].row[i]);
      }

      bool showCheatBanner = (millis() < g_cheatAlertExpiry);

      if (showCheatBanner) {
        fb.fillRect(0, 70, TFT_W, 10, ST7735_RED);
        fb.setTextColor(ST7735_WHITE, ST7735_RED);
        fb.setCursor(20, 71);
        fb.setTextSize(1);
        fb.print("CHEAT RECORDED");
      }

      xSemaphoreGive(mTft);

      if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(40)) == pdTRUE) {
        tft.drawRGBBitmap(0, 0, fb.getBuffer(), TFT_W, TFT_H);
        xSemaphoreGive(mSpi);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
// ============================================
// ANTI-CHEAT TASK
// ============================================
void taskAntiCheat(void *pv)
{
  uint32_t coverStart   = 0;
  bool     wasCovered   = false;
  for (;;) {
    bool hasAmbient = digitalRead(IR_AMBIENT_PIN) == HIGH;
    bool isCovered  = !hasAmbient;

    if (isCovered) {
      if (!wasCovered) {
        coverStart = millis();
        wasCovered = true;
      } else if (!g_cheatDetected && millis() - coverStart >= CHEAT_DETECTION_MS) {
        g_cheatDetected   = true;
        g_cheatAlertExpiry = millis() + CHEAT_ALERT_DURATION;

        logSd("[CHEAT] IR covered  >5s — CHEAT DETECTED! ");
        Serial.println("[CHEAT] DETECTED! Sending immediate GPRS... ");

        if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
          g_pg[2].row[5] = "Evt: CHEAT! ";
          xSemaphoreGive(mTft);
        }
        if (xSemaphoreTake(mGprs, MTX) == pdTRUE) {
          g_gprs.lastEvt = "CHEAT ";
          xSemaphoreGive(mGprs);
        }

        for (uint8_t i = 0; i < IR_IMMEDIATE_SEND; i++) {
          sendImmediateGprsPayload(true);
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      }
    } else {
      wasCovered  = false;
      if (g_cheatDetected) {
        g_cheatDetected = false;
        logSd("[CHEAT] IR uncovered — flag cleared ");
        Serial.println("[CHEAT] IR uncovered, cheat flag cleared. ");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(CHEAT_CHECK_INTERVAL));
  }
}
// ============================================
// GPS TASK
// ============================================
void taskGps(void *pv)
{
  for (;;) {
    int n = Serial1.available();
    for (int i = 0; i < n; i++) GPS.encode(Serial1.read());
    bool hasT = GPS.time.isValid(), hasL = GPS.location.isValid();

    if (xSemaphoreTake(mGps, MTX) == pdTRUE) {
      g_gps.valid    = hasT || hasL;
      g_gps.locValid = hasL;
      g_gps.sat      = GPS.satellites.value();
      if (hasL) { g_gps.lat = GPS.location.lat(); g_gps.lng = GPS.location.lng(); g_gps.alt = GPS.altitude.meters(); }
      if (hasT) { g_gps.h = GPS.time.hour(); g_gps.m = GPS.time.minute(); g_gps.s = GPS.time.second(); }
      xSemaphoreGive(mGps);
    }

    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      int8_t dh = 0;
      if (hasT) {
        float lon = g_gps.locValid ? g_gps.lng : 107.0f;
        int utcOffset = (lon < 115.0f) ? 7 : (lon < 134.0f) ? 8 : 9;
        dh = (int8_t)((g_gps.h + utcOffset + 24) % 24);
      }
      char tb[12]; snprintf(tb, 12, "%02d:%02d:%02d ", hasT ? dh : 0, hasT ? g_gps.m : 0, hasT ? g_gps.s : 0);

      g_pg[1].row[0] = "GPS Status ";
      g_pg[1].row[1] = hasL ? ("Lat:  " + String(g_gps.lat, 6))       : "Lat: -";
      g_pg[1].row[2] = hasL ? ("Long:  " + String(g_gps.lng, 6))      : "Long: -";
      g_pg[1].row[3] = hasL ? ("Alt:  " + String(g_gps.alt, 1) + "m ") : "Alt: -";
      g_pg[1].row[4] = "Sat:  " + String(g_gps.sat);
      g_pg[1].row[5] = "Time:  " + String(tb);
      xSemaphoreGive(mTft);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
// ============================================
// LORAWAN TASK
// ============================================
#if ENABLE_LORAWAN
void taskLora(void *pv)
{
  vTaskDelay(pdMS_TO_TICKS(500));
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
  delay(100);
  auto setLoraPage = [](const char* ev, int16_t rssi, float snr, bool joined) {
    if (xSemaphoreTake(mTft, pdMS_TO_TICKS(30)) == pdTRUE) {
      g_pg[0].row[0] = "LoRaWAN Status ";
      g_pg[0].row[1] = "Event:  " + String(ev);
      g_pg[0].row[2] = "RSSI:  " + String(rssi) + " dBm ";
      g_pg[0].row[3] = "SNR:  " + String(snr, 1) + " dB ";
      g_pg[0].row[4] = "NA ";
      g_pg[0].row[5] = joined ? "Joined: YES " : "Joined: NO ";
      xSemaphoreGive(mTft);
    }
  };

  if (radio.begin() != RADIOLIB_ERR_NONE) { setLoraPage("FAIL ", 0, 0, false); vTaskDelete(NULL); }
  if (node.beginOTAA(joinEUI, devEUI, nwkKey, appKey) != RADIOLIB_ERR_NONE) { setLoraPage("INIT_FAIL ", 0, 0, false); vTaskDelete(NULL); }

  for (uint8_t a = 1; a <= MAX_JOIN_ATTEMPTS; a++) {
    char jm[14]; snprintf(jm, 14, "JOIN%d/%d ", a, MAX_JOIN_ATTEMPTS);
    setLoraPage(jm, 0, 0, false);
    int16_t s = node.activateOTAA();
    vTaskDelay(pdMS_TO_TICKS(10));
    if (s == RADIOLIB_LORAWAN_NEW_SESSION) {
      Serial.println("[LoRa] Connected ");
      if (xSemaphoreTake(mLora, MTX) == pdTRUE) { g_lora.joined = true; xSemaphoreGive(mLora); }
      setLoraPage("JOINED ", 0, 0, true);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(JOIN_RETRY_MS));
  }
  if (!g_lora.joined) {
    Serial.println("[LoRa] No join ");
    setLoraPage("NO_JOIN ", 0, 0, false);
    vTaskDelete(NULL);
  }

  uint32_t tUp = 0;
  for (;;) {
    if (irSend || millis() - tUp >= UPLINK_INTERVAL_MS) {
      irSend = false;
      Payload pl = {};
      if (xSemaphoreTake(mGps, MTX) == pdTRUE) { pl.sat = g_gps.sat; pl.lat = g_gps.lat; pl.lng = g_gps.lng; pl.alt = g_gps.alt; xSemaphoreGive(mGps); }
      if (irPend) { pl.shooter_sub = 1; pl.shooter_addr = irAddr; pl.sub_id = irCmd; pl.status = 1; irPend = false; }
      else if (xSemaphoreTake(mIr, MTX) == pdTRUE) {
        pl.shooter_addr = g_ir.got ? g_ir.addr : 0;
        pl.sub_id       = g_ir.got ? g_ir.cmd  : 0;
        pl.shooter_sub  = g_ir.got ? 1 : 0;
        pl.status       = g_ir.got ? 1 : 0;
        xSemaphoreGive(mIr);
      }
      if (xSemaphoreTake(mLora, MTX) == pdTRUE) { pl.rssi = g_lora.rssi; pl.snr = (int8_t)(g_lora.snr + 0.5f); xSemaphoreGive(mLora); }
      pl.cheatDetected = g_cheatDetected ? 1 : 0;

      uint8_t buf[sizeof(Payload)]; memcpy(buf, &pl, sizeof(pl));
      int16_t s = node.sendReceive(buf, sizeof(buf));

      String ev; float snr = 0; int16_t rssi = 0;
      if (s < RADIOLIB_ERR_NONE) { ev = "TX_FAIL "; }
      else {
        ev = s > 0 ? "TX+RX_OK " : "TX_OK ";
        snr  = radio.getSNR();
        rssi = radio.getRSSI();
        if (xSemaphoreTake(mIr, MTX) == pdTRUE) { g_ir.got = false; xSemaphoreGive(mIr); }
      }
      if (xSemaphoreTake(mLora, MTX) == pdTRUE) { g_lora.ev = ev; g_lora.snr = snr; g_lora.rssi = rssi; xSemaphoreGive(mLora); }
      setLoraPage(ev.c_str(), rssi, snr, true);
      Serial.printf("[LoRa] %s RSSI:%d SNR:%.1f Cheat:%d\n", ev.c_str(), rssi, snr, pl.cheatDetected);
      tUp = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
#endif
// ============================================
// WIFI TASK
// ============================================
#if ENABLE_WIFI
void taskWifi(void *pv)
{
  WiFi.disconnect(true); vTaskDelay(pdMS_TO_TICKS(200));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000)
    vTaskDelay(pdMS_TO_TICKS(500));

  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Serial.printf("[WiFi] Connected IP: %s\n", ip.c_str());
    if (xSemaphoreTake(mWifi, MTX) == pdTRUE) { g_wifi.connected = true; g_wifi.ip = ip; xSemaphoreGive(mWifi); }
    sdUpload();
  } else {
    Serial.println("[WiFi] Connect failed ");
  }

  uint32_t tUp = 0;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      if (xSemaphoreTake(mWifi, MTX) == pdTRUE) { g_wifi.connected = false; xSemaphoreGive(mWifi); }
      if (millis() - g_wifi.lastRC >= 10000) {
        g_wifi.lastRC = millis();
        WiFi.disconnect(true); vTaskDelay(pdMS_TO_TICKS(300));
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        unsigned long tw = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - tw < 15000) vTaskDelay(pdMS_TO_TICKS(500));
        if (WiFi.status() == WL_CONNECTED) {
          String ip = WiFi.localIP().toString();
          Serial.printf("[WiFi] Reconnected IP: %s\n", ip.c_str());
          if (xSemaphoreTake(mWifi, MTX) == pdTRUE) { g_wifi.connected = true; g_wifi.ip = ip; xSemaphoreGive(mWifi); }
        }
      }
    } else {
      if (xSemaphoreTake(mWifi, MTX) == pdTRUE) {
        if (!g_wifi.connected) { g_wifi.connected = true; g_wifi.ip = WiFi.localIP().toString(); }
        xSemaphoreGive(mWifi);
      }
    }

    if (g_wifi.connected && millis() - tUp >= WIFI_UPLOAD_MS) {
      float lat = 0, lng = 0;
      if (xSemaphoreTake(mGps, MTX) == pdTRUE) { lat = g_gps.lat; lng = g_gps.lng; xSemaphoreGive(mGps); }
      wifiPost(lat, lng);
      tUp = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void wifiPost(float lat, float lng)
{
  char irBuf[32] = "-"; float alt = 0; uint8_t sat = 0; int16_t rssi = 0; float snr = 0;
  uint16_t ia = 0; uint8_t ic = 0;
  const char* hitStatus = "NONE";
  if (xSemaphoreTake(mIr, MTX) == pdTRUE) {
    if (g_ir.got) { snprintf(irBuf, 32, "HIT:0x%X-0x%X", g_ir.addr, g_ir.cmd); ia = g_ir.addr; ic = g_ir.cmd; hitStatus = "HIT"; }
    xSemaphoreGive(mIr);
  }
  if (xSemaphoreTake(mGps, MTX) == pdTRUE) { alt = g_gps.alt; sat = g_gps.sat; xSemaphoreGive(mGps); }
  if (xSemaphoreTake(mLora, MTX) == pdTRUE) { rssi = g_lora.rssi; snr = g_lora.snr; xSemaphoreGive(mLora); }
  char pl[450];
  snprintf(pl, sizeof(pl),
           "{ \"deviceId\": \"%s\", \"lat\":%.6f, \"lng\":%.6f, \"alt\":%.1f, "
           "\"sats\":%u, \"rssi\":%d, \"snr\":%.1f, \"battery\":0, "
           "\"irAddress\":%u, \"irCommand\":%u, \"hitStatus\": \"%s\", \"cheatDetected\":%d}",
           DEVICE_ID, lat, lng, alt, sat, rssi, snr, ia, ic, hitStatus, g_cheatDetected ? 1 : 0);

  bool okVercel = false; int codeVercel = 0;
  if (WiFi.status() == WL_CONNECTED) {
    for (int i = 1; i <= 3 && !okVercel; i++) {
      HTTPClient http; http.begin(VERCEL_API_URL);
      http.addHeader("Content-Type", "application/json"); http.setTimeout(10000);
      codeVercel = http.POST(pl); okVercel = (codeVercel == 200 || codeVercel == 201); http.end();
      if (!okVercel && i < 3) vTaskDelay(pdMS_TO_TICKS(2000));
    }
    Serial.printf("[WiFi] Vercel: %s (code:%d)\n", okVercel ? "OK " : "FAIL ", codeVercel);
  }
  if (!okVercel) sdSave(lat, lng, alt, sat, 0, rssi, snr, irBuf);
}
#endif
// ============================================
// GPRS SIM900A
// ============================================
#if ENABLE_GPRS
void buildDataPayload(Payload &payload)
{
  payload.addr_id     = 0x12345678;
  payload.sub_id      = random(0, 256);
  payload.status      = 1;
  if (xSemaphoreTake(mGps, MTX) == pdTRUE) {
    payload.lat = g_gps.lat;
    payload.lng = g_gps.lng;
    payload.alt = g_gps.alt;
    xSemaphoreGive(mGps);
  } else {
    payload.lat = payload.lng = payload.alt = 0.0f;
  }

  if (xSemaphoreTake(mIr, MTX) == pdTRUE) {
    payload.shooter_addr = g_ir.got ? g_ir.addr : 0;
    payload.shooter_sub  = g_ir.got ? g_ir.cmd  : 0;
    payload.sub_id       = g_ir.got ? g_ir.cmd  : 0;
    xSemaphoreGive(mIr);
  } else {
    payload.shooter_addr = 0;
    payload.shooter_sub  = 0;
  }

  payload.cheatDetected = g_cheatDetected ? 1 : 0;
}

void initGprsHardware()
{
  Serial.println("Initializing GPRS modem...");
  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, HIGH);
  delay(200);
  digitalWrite(GPRS_RST_PIN, LOW);
  delay(200);
  digitalWrite(GPRS_RST_PIN, HIGH);
  delay(1000);
  sim900.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
  delay(1000);
}

void taskGprsLoop(void *pv)
{
  if (!modem.begin()) {
    String err = "GPRS init failed";
    Serial.println(err);
    logSd(err.c_str());
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[1] = "Conn: FAIL";
      g_pg[2].row[5] = "Evt: NO_MODEM";
      xSemaphoreGive(mTft);
    }
    vTaskDelete(NULL);
  }
  if (strlen(GPRS_SIM_PIN) > 0 && !modem.simUnlock(GPRS_SIM_PIN)) {
    String err = "Failed to unlock SIM ";
    Serial.println(err);
    logSd(err.c_str());
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[5] = "Evt: SIM_FAIL ";
      xSemaphoreGive(mTft);
    }
    vTaskDelete(NULL);
  }

  bool gprsConnected = false;
  for (int retry = 0; retry < 5; retry++) {
    if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
      gprsConnected = true;
      String ip = modem.getLocalIP();
      logSd(("GPRS: Connected persistently, IP=" + ip).c_str());
      Serial.println("GPRS: Connected persistently, IP=" + ip);
      if (xSemaphoreTake(mGprs, MTX) == pdTRUE) { g_gprs.connected = true; g_gprs.ip = ip; xSemaphoreGive(mGprs); }
      if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
        g_pg[2].row[1] = "Conn: YES ";
        g_pg[2].row[2] = "IP:  " + ip.substring(0, min(14, (int)ip.length()));
        g_pg[2].row[5] = "Evt: CONNECTED ";
        xSemaphoreGive(mTft);
      }
      break;
    }
    logSd(("GPRS: Connect retry  " + String(retry + 1)).c_str());
    delay(5000);
  }

  if (!gprsConnected) {
    logSd("GPRS: Failed to establish persistent connection ");
    Serial.println("GPRS: Failed to establish persistent connection ");
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[1] = "Conn: NO ";
      g_pg[2].row[5] = "Evt: CONN_FAIL ";
      xSemaphoreGive(mTft);
    }
  }

  uint32_t lastScheduledSend = 0;
  for (;;) {
    if (!gprsConnected || !modem.isGprsConnected()) {
      logSd("GPRS: Connection lost — attempting reconnect ");
      if (modem.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
        gprsConnected = true;
        String ip = modem.getLocalIP();
        logSd(("GPRS: Reconnected, IP=" + ip).c_str());
        if (xSemaphoreTake(mGprs, MTX) == pdTRUE) { g_gprs.connected = true; g_gprs.ip = ip; xSemaphoreGive(mGprs); }
        if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
          g_pg[2].row[1] = "Conn: YES ";
          g_pg[2].row[2] = "IP:  " + ip.substring(0, min(14, (int)ip.length()));
          g_pg[2].row[5] = "Evt: RECONNECTED ";
          xSemaphoreGive(mTft);
        }
      } else {
        gprsConnected = false;
        if (xSemaphoreTake(mGprs, MTX) == pdTRUE) { g_gprs.connected  = false; xSemaphoreGive(mGprs); }
        if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
          g_pg[2].row[1] = "Conn: NO ";
          g_pg[2].row[5] = "Evt: RCNN_FAIL ";
          xSemaphoreGive(mTft);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        continue;
      }
    }

    if (millis() - lastScheduledSend >= GPRS_INTERVAL_MS) {
      Payload payload;
      buildDataPayload(payload);

      String event      = "HTTP_FAIL ";
      String responseLine = "-";

      if (gsmClient.connect(GPRS_HOST, GPRS_PORT)) {
        String url = String(GPRS_PATH) + "?api_key=" + GPRS_API_KEY;
        gsmClient.print(String("POST ") + url + " HTTP/1.1\r\n");
        gsmClient.print(String("Host: ") + GPRS_HOST + "\r\n");
        gsmClient.print("Content-Type: application/octet-stream\r\n");
        gsmClient.print("Content-Length: " + String(sizeof(Payload)) + "\r\n");
        gsmClient.print("Connection: close\r\n\r\n");
        gsmClient.write((uint8_t *) &payload, sizeof(payload));

        unsigned long timeout = millis();
        String response = " ";
        while (gsmClient.available() == 0) {
          if (millis() - timeout > 5000) break;
        }
        if (gsmClient.available()) {
          response = gsmClient.readString();
          if (response.indexOf("OK") >= 0) {
            event = "DB_OK ";
          } else {
            event = "DB_FAIL ";
          }
          responseLine = response.substring(0, min(50, (int)response.length()));
        }
        gsmClient.stop();
      }

      logSd(("SCHEDULED GPRS: " + event + " | Resp: " + responseLine).c_str());
      Serial.println("SCHEDULED GPRS: " + event);

      if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
        g_pg[2].row[0] = "GPRS/DB Status ";
        g_pg[2].row[1] = "Conn: YES ";
        g_pg[2].row[2] = "IP:  " + modem.getLocalIP().substring(0, 14);
        String displayResp = responseLine;
        if (displayResp.length() > 10) displayResp = displayResp.substring(0, 10);
        g_pg[2].row[4] = "Resp:  " + displayResp;
        g_pg[2].row[5] = "Evt:  " + event;
        xSemaphoreGive(mTft);
      }
      if (xSemaphoreTake(mGprs, MTX) == pdTRUE) { g_gprs.lastEvt = event; xSemaphoreGive(mGprs); }

      lastScheduledSend = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void sendImmediateGprsPayload(bool isCheat)
{
  if (!modem.isGprsConnected()) {
    logSd("IMMEDIATE GPRS: Not connected");
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[5] = "Evt: NO_GPRS";
      xSemaphoreGive(mTft);
    }
    return;
  }
  Payload payload;
  buildDataPayload(payload);

  const char* path = isCheat ? GPRS_CHEAT_PATH : GPRS_PATH;

  if (gsmClient.connect(GPRS_HOST, GPRS_PORT)) {
    String url = String(path) + "?api_key=" + GPRS_API_KEY;
    gsmClient.print("POST " + url + " HTTP/1.1\r\n");
    gsmClient.print("Host: " + String(GPRS_HOST) + "\r\n");
    gsmClient.print("Content-Type: application/octet-stream\r\n");
    gsmClient.print("Content-Length: " + String(sizeof(Payload)) + "\r\n");
    gsmClient.print("Connection: close\r\n");
    gsmClient.print("\r\n");
    gsmClient.write((uint8_t*) &payload, sizeof(payload));

    String response = " ";
    unsigned long start = millis();
    while (millis() - start < 5000 && gsmClient.available() == 0) vTaskDelay(1);
    while (gsmClient.available()) response += (char)gsmClient.read();
    gsmClient.stop();

    String event = (response.indexOf("OK") >= 0)
                   ? (isCheat ? "CHEAT_OK " : "IR_DB_OK ")
                   : (isCheat ? "CHEAT_FAIL " : "IR_DB_FAIL ");

    logSd(("IMMEDIATE GPRS: " + event + " | Resp: " + response.substring(0, 30)).c_str());
    Serial.println("IMMEDIATE GPRS: " + event);

    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[5] = "Evt:  " + event;
      xSemaphoreGive(mTft);
    }
    if (xSemaphoreTake(mGprs, MTX) == pdTRUE) { g_gprs.lastEvt = event; xSemaphoreGive(mGprs); }

  } else {
    logSd("IMMEDIATE GPRS: TCP connect failed ");
    Serial.println("IMMEDIATE GPRS: TCP connect failed ");
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      g_pg[2].row[5] = "Evt: CONN_FAIL ";
      xSemaphoreGive(mTft);
    }
  }
}
#endif
// ============================================
// IR TASK
// ============================================
void taskIr(void *pv)
{
  for (;;) {
    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.protocol == NEC) {
        uint16_t a = IrReceiver.decodedIRData.address;
        uint8_t  c = IrReceiver.decodedIRData.command;
        if (xSemaphoreTake(mIr, MTX) == pdTRUE) {
          g_ir.got = true; g_ir.addr = a; g_ir.cmd = c; g_ir.t = millis();
          g_ir.hitCount++;
          irPend = true; irAddr = a; irCmd = c; irSend = true;
          xSemaphoreGive(mIr);
        }

        Serial.printf("[IR] Addr: 0x%04X  Command: 0x%02X\n", a, c);
        char buf[48]; snprintf(buf, 48, "[IR] HIT Addr:0x%04X Cmd:0x%02X ", a, c);
        logSd(buf);

        uint32_t hc = 0;
        if (xSemaphoreTake(mIr, MTX) == pdTRUE) { hc = g_ir.hitCount; xSemaphoreGive(mIr); }
        char tBuf[10]; snprintf(tBuf, 10, "%02lu:%02lu ", (millis() / 60000) % 60, (millis() / 1000) % 60);

        if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
          g_pg[3].row[0] = "IR Hit Info ";
          g_pg[3].row[1] = "Shooter: 0x" + String(a, HEX);
          g_pg[3].row[2] = "Sub: 0x" + String(c, HEX);
          g_pg[3].row[3] = "Loc: VEST ";
          g_pg[3].row[4] = "Time:  " + String(tBuf);
          g_pg[3].row[5] = "Hits:  " + String(hc);
          xSemaphoreGive(mTft);
        }

#if ENABLE_GPRS
        for (uint8_t i = 0; i < IR_IMMEDIATE_SEND; i++) {
          sendImmediateGprsPayload(false);
          vTaskDelay(pdMS_TO_TICKS(200));
        }
#endif
      }
      IrReceiver.resume();
    }
    if (xSemaphoreTake(mTft, MTX) == pdTRUE) {
      if (xSemaphoreTake(mIr, MTX) == pdTRUE) {
        if (!g_ir.got) {
          g_pg[3].row[1] = "Shooter: -";
          g_pg[3].row[2] = "Sub: -";
          g_pg[3].row[3] = "Loc: NONE";
          g_pg[3].row[4] = "Time: --:--";
        }
        xSemaphoreGive(mIr);
      }
      xSemaphoreGive(mTft);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
// ============================================
// SD TASK
// ============================================
#if ENABLE_SD
void taskSd(void *pv)
{
  vTaskDelay(pdMS_TO_TICKS(3000));
  bool ok = false;
  if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(500)) == pdTRUE) {
    ok = SD.begin(SD_CS, SPI, 8000000);
    xSemaphoreGive(mSpi);
  }
  if (!ok) { Serial.println("[SD] Init fail"); vTaskDelete(NULL); }
  sdOK = true;
  Serial.printf("[SD] %luMB connected\n", (unsigned long)(SD.cardSize() / (1024 * 1024)));
  if (SD.exists("/tracker.log")) SD.remove("/tracker.log");
  uint32_t tW = 0;
  for (;;) {
    if (millis() - tW >= SD_WRITE_MS) {
      if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(100)) == pdTRUE) {
        File f = SD.open("/tracker.log", FILE_APPEND);
        if (f) {
          char* msg;
          while (xQueueReceive(qLog, &msg, 0) == pdTRUE) { f.println(msg); delete[] msg; }
          f.close();
        }
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
void logSd(const char* msg)
{
#if ENABLE_SD
  char* c = new char[strlen(msg) + 1]; strcpy(c, msg);
  if (xQueueSendToBack(qLog, &c, pdMS_TO_TICKS(10)) != pdTRUE) delete[] c;
#endif
}
void sdSave(float lat, float lng, float alt, uint8_t sat, uint16_t bat, int16_t rssi, float snr, const char* ir)
{
#if ENABLE_SD
  if (!sdOK) return;
  if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(100)) == pdTRUE) {
    File f = SD.open("/offline_queue.csv", FILE_APPEND);
    if (f) {
      char ln[200]; snprintf(ln, 200, "%lu,%.6f,%.6f,%.1f,%u,%u,%d,%.1f,%s", millis(), lat, lng, alt, sat, bat, rssi, snr, ir);
      f.println(ln); f.close();
    }
    xSemaphoreGive(mSpi);
  }
#endif
}
bool sdUpload()
{
#if ENABLE_WIFI
  if (!sdOK) return false;
  bool ex = false;
  if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(200)) == pdTRUE) { ex = SD.exists("/offline_queue.csv"); xSemaphoreGive(mSpi); }
  if (!ex) return false;
  int okCnt = 0; String fail = " ";
  if (xSemaphoreTake(mSpi, pdMS_TO_TICKS(200)) == pdTRUE) {
    File f = SD.open("/offline_queue.csv", FILE_READ);
    if (!f) { xSemaphoreGive(mSpi); return false; }
    while (f.available()) {
      String ln = f.readStringUntil('\n'); ln.trim(); if (ln.length() < 10) continue;
      int c[8], cc = 0; for (size_t i = 0; i < ln.length() && cc < 8; i++) if (ln.charAt(i) == ',') c[cc++] = i;
      if (cc < 7) continue;
      float lat = ln.substring(c[0] + 1, c[1]).toFloat(), lng = ln.substring(c[1] + 1, c[2]).toFloat(), alt = ln.substring(c[2] + 1, c[3]).toFloat();
      uint8_t sat = ln.substring(c[3] + 1, c[4]).toInt(); uint16_t bat = ln.substring(c[4] + 1, c[5]).toInt();
      int16_t rssi = ln.substring(c[5] + 1, c[6]).toInt(); float snr = ln.substring(c[6] + 1, c[7]).toFloat();
      String ir = ln.substring(c[7] + 1);
      xSemaphoreGive(mSpi);
      HTTPClient http; http.begin(VERCEL_API_URL); http.addHeader("Content-Type", "application/json"); http.setTimeout(10000);
      char pl[400]; snprintf(pl, 400, "{ \"source\": \"wifi_sd\", \"id\": \"%s\", \"lat\":%.6f, \"lng\":%.6f, \"alt\":%.1f, \"irStatus\": \"%s\", \"battery\":%u, \"satellites\":%u, \"rssi\":%d, \"snr\":%.1f}", DEVICE_ID, lat, lng, alt, ir.c_str(), bat, sat, rssi, snr);
      int code = http.POST(pl); http.end();
      if (code == 200 || code == 201) okCnt++; else fail += ln + "\n";
      vTaskDelay(pdMS_TO_TICKS(500));
      xSemaphoreTake(mSpi, pdMS_TO_TICKS(200));
    }
    f.close();
    if (fail.length() == 0) SD.remove("/offline_queue.csv");
    else { File fw = SD.open("/offline_queue.csv", FILE_WRITE); if (fw) { fw.print(fail); fw.close(); } }
    xSemaphoreGive(mSpi);
  }
  return okCnt > 0;
#else
  return false;
#endif
}
String decodeState(int16_t r)
{
  switch (r) {
    case RADIOLIB_ERR_NONE:                 return "OK ";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:       return "NO_CHIP ";
    case RADIOLIB_ERR_NETWORK_NOT_JOINED:   return "NOT_JOIN ";
    case RADIOLIB_ERR_NO_JOIN_ACCEPT:       return "NO_ACCEPT ";
    case RADIOLIB_LORAWAN_NEW_SESSION:      return "NEW_SESS ";
    case RADIOLIB_LORAWAN_SESSION_RESTORED: return "RESTORED ";
    default: return "ERR(" + String(r) + ") ";
  }
}
