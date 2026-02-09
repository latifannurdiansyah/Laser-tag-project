#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <TinyGPS++.h>
#include <IRremote.hpp>
#include <RadioLib.h>

// --- Konfigurasi LoRaWAN ---
// Ganti dengan kredensial dari TTN Console
uint64_t devEUI = 0x70B3D57ED007368B;
uint64_t joinEUI = 0x0000000000000000;
uint8_t appKey[] = { 0x6F, 0x90, 0xBD, 0x2B, 0x94, 0xD9, 0x50, 0x8B, 0xB4, 0x69, 0x14, 0x81, 0xED, 0xF0, 0xC7, 0x6C };
uint8_t nwkKey[] = { 0xD3, 0xC8, 0xC0, 0xBE, 0x77, 0x5F, 0x4C, 0x7E, 0x8A, 0xA2, 0x19, 0x89, 0x9F, 0xF8, 0x86, 0xB8 };

// Pinout Heltec Tracker V1.1 (SX1262)
#define RADIO_CS_PIN    8
#define RADIO_DIO1_PIN  14
#define RADIO_RST_PIN   12
#define RADIO_BUSY_PIN  13

// Pinout Lainnya
#define Vext_Ctrl 3
#define IR_RECEIVE_PIN 5
#define GPS_RX 33
#define GPS_TX 34
#define SD_CS  4
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39

// Objek Global
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &AS923); // Gunakan AS923 untuk Indonesia
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, 42, 41, TFT_RST); // MOSI 42, SCLK 41
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);

// Timer
unsigned long lastLoRaSend = 0;
const uint32_t LORA_INTERVAL = 30000; // Kirim tiap 30 detik (patuhi Fair Use Policy)

bool isJoined = false;
bool sdCardAvailable = false;
bool irDataReceived = false;
uint16_t lastIRAddress = 0;
uint8_t lastIRCommand = 0;

// --- Fungsi Helper LoRaWAN ---
void joinLoRaWAN() {
  Serial.println(F("[LoRaWAN] Mencoba Join..."));
  int16_t state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state == RADIOLIB_ERR_NONE) {
    isJoined = true;
    Serial.println(F("[LoRaWAN] Join Berhasil!"));
  } else {
    Serial.printf("[LoRaWAN] Join Gagal, kode: %d\n", state);
  }
}

// Mengirim data GPS + IR Status
void sendLoRaData() {
  if (!isJoined) return;

  Serial.println(F("[LoRaWAN] Menyiapkan Uplink..."));
  
  // Format Payload Sederhana (8 bytes)
  // Lat (4b), Lng (4b) - dikali 1000000 agar jadi integer
  int32_t lat = gps.location.lat() * 1000000;
  int32_t lng = gps.location.lng() * 1000000;
  
  uint8_t payload[9];
  payload[0] = (lat >> 24) & 0xFF;
  payload[1] = (lat >> 16) & 0xFF;
  payload[2] = (lat >> 8) & 0xFF;
  payload[3] = lat & 0xFF;
  payload[4] = (lng >> 24) & 0xFF;
  payload[5] = (lng >> 16) & 0xFF;
  payload[6] = (lng >> 8) & 0xFF;
  payload[7] = lng & 0xFF;
  payload[8] = irDataReceived ? lastIRCommand : 0;

  int16_t state = node.sendReceive(payload, 9);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[LoRaWAN] Uplink Berhasil Terkirim"));
    irDataReceived = false; // Reset status IR setelah terkirim
  } else {
    Serial.printf("[LoRaWAN] Uplink Gagal: %d\n", state);
  }
}

void setup() {
  // Power On Peripherals
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH); 
  
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); // GPS biasanya 9600
  
  // Init TFT
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  
  // Init IR
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  // Init SD Card (SPI Hardware khusus Heltec Tracker)
  SPI.begin(9, 11, 10, SD_CS); // SCK, MISO, MOSI, CS
  if (SD.begin(SD_CS)) {
    sdCardAvailable = true;
    Serial.println("SD OK");
  }

  // Init Radio & LoRaWAN
  int16_t state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    joinLoRaWAN();
  } else {
    Serial.println("Radio Hardware Error!");
  }
}

void loop() {
  // 1. Proses GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // 2. Proses IR
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == NEC) {
      lastIRAddress = IrReceiver.decodedIRData.address;
      lastIRCommand = IrReceiver.decodedIRData.command;
      irDataReceived = true;
      Serial.printf("IR Hit: 0x%02X\n", lastIRCommand);
    }
    IrReceiver.resume();
  }

  // 3. Proses LoRaWAN Uplink
  if (isJoined && (millis() - lastLoRaSend >= LORA_INTERVAL)) {
    if (gps.location.isValid()) {
      sendLoRaData();
      lastLoRaSend = millis();
    }
  }

  // 4. Update Display (Sederhana)
  static unsigned long lastDisp = 0;
  if (millis() - lastDisp > 1000) {
    framebuffer.fillScreen(ST7735_BLACK);
    framebuffer.setCursor(5, 5);
    framebuffer.setTextColor(ST7735_WHITE);
    framebuffer.printf("Sats: %d", gps.satellites.value());
    framebuffer.setCursor(5, 20);
    framebuffer.printf("LoRa: %s", isJoined ? "JOINED" : "TRYING");
    framebuffer.setCursor(5, 35);
    framebuffer.printf("IR: %s", irDataReceived ? "HIT!" : "-");
    
    tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
    lastDisp = millis();
  }
}