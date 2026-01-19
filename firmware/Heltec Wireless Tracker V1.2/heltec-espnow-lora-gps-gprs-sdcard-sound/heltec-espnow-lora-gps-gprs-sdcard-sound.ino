#include "config.h"

// Global objects
TinyGPSPlus GPS;
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(TFT_WIDTH, TFT_HEIGHT);

// GPS data
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

// LoRaWAN status
LoraStatus g_loraStatus = {
  .lastEvent = "IDLE",
  .rssi = 0,
  .snr = 0.0f,
  .joined = false
};

// TFT pages
TftPageData g_TftPageData[TFT_MAX_PAGES] = {
  {"LoRaWAN Status", "-", "-", "-", "-", "-"},
  {"GPS Status", "Lat: -", "Long: -", "Alt: -", "Sat: -", "Time: --:--:--"}
};

// FreeRTOS handles
SemaphoreHandle_t xGpsMutex = NULL;
SemaphoreHandle_t xLoraMutex = NULL;
SemaphoreHandle_t xTftMutex = NULL;
QueueHandle_t xLogQueue = NULL;

// ESP-NOW
uint8_t senderMac[] = {0x10, 0x20, 0xBA, 0x65, 0x47, 0x34};
ir_data_t latestIr = {0, 0, 0};
unsigned long irTimestamp = 0;
uint8_t lastSeq = 0;
uint32_t lastSeqTime = 0;
bool hitDetected = false;

void setup() {
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  delay(100);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);

  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("\n✅ Heltec Tracker Full System Started!");

  // GPS
  Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GNSS module powered and Serial1 started.");

  // TFT
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  // SD
  if (!SD.begin(SD_CS, SPI, 8000000)) {
    Serial.println("SD Mount Failed!");
  } else {
    Serial.println("SD Mounted");
  }

  // Mutexes
  xGpsMutex = xSemaphoreCreateMutex();
  xLoraMutex = xSemaphoreCreateMutex();
  xTftMutex = xSemaphoreCreateMutex();

  // Log queue
  xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(String *));
  if (!xLogQueue) {
    Serial.println("❌ Failed to create log queue!");
    while (1) delay(1000);
  }

  // ESP-NOW
  espNowInit();

  // Tasks
  xTaskCreatePinnedToCore(gpsTask, "GPS", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(loraTask, "LoRa", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(tftTask, "TFT", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sdCardTask, "SD", 4096, NULL, 1, NULL, 0);

  logToSd("=== System Boot ===");
}

void loop() {
  delay(1000); // Main loop idle
}