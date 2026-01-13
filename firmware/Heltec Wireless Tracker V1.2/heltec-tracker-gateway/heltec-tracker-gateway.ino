#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <RadioLib.h>

// === Pin Configuration ===
#define VEXT_CTRL 3
#define LED_BUILTIN 18

#define TFT_LED_K 21
#define TFT_CS 38
#define TFT_DC 40
#define TFT_RST 39
#define TFT_SCLK 41
#define TFT_MOSI 42

// === LoRa Pins (SX1262) ===
#define RADIO_CS_PIN 8
#define RADIO_DIO1_PIN 14
#define RADIO_RST_PIN 12
#define RADIO_BUSY_PIN 13
#define RADIO_SCLK_PIN 9
#define RADIO_MISO_PIN 11
#define RADIO_MOSI_PIN 10
#define LORA_LED_PIN LED_BUILTIN

// === LoRa Configuration (HARUS SAMA DENGAN SENDER) ===
#define LORA_FREQUENCY 923.0
#define LORA_BANDWIDTH 125.0
#define LORA_SPREADING_FACTOR 10
#define LORA_CODING_RATE 5
#define LORA_SYNC_WORD 0x12
#define LORA_PREAMBLE_LEN 8
#define LORA_CRC true

#define GROUP_ID 0x01
#define MAX_NODES 10
#define NODE_TIMEOUT_SEC 30
#define DISPLAY_CYCLE_MS 3000

// === Data Structures ===
typedef struct __attribute__((packed)) {
  uint8_t  header;        // 0xAA
  uint8_t  groupId;       // GROUP_ID
  uint8_t  nodeId;        // NODE_ID
  int32_t  lat;           // latitude * 1000000
  int32_t  lng;           // longitude * 1000000
  int16_t  alt;           // altitude
  uint8_t  satellites;    // GPS satellites
  uint16_t battery;       // battery voltage (mV)
  uint32_t uptime;        // uptime in seconds
  uint16_t irAddress;     // IR address
  uint8_t  irCommand;     // IR command
  uint8_t  hitStatus;     // 1=HIT, 0=NO HIT
  uint8_t  checksum;      // XOR checksum
} lora_payload_t;

typedef struct {
  uint8_t  nodeId;
  bool     valid;
  uint32_t lastSeen;      // Unix timestamp (seconds)
  int16_t  lastRSSI;
  float    lastSNR;
  uint16_t lastBattery;
  float    lat;
  float    lng;
  int16_t  alt;
  uint8_t  satellites;
  uint16_t irAddress;
  uint8_t  irCommand;
  uint8_t  hitStatus;
  uint32_t rxCount;
  uint32_t lostPackets;   // Untuk deteksi packet loss
  uint32_t lastPacketId;  // Track packet ID
} node_stats_t;

typedef struct {
  bool     initialized;
  uint32_t rxCount;
  uint32_t rxErrors;
  uint32_t checksumErrors;
  uint32_t duplicates;
  int16_t  lastRSSI;
  float    lastSNR;
} lora_status_t;

// === Global Objects ===
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// === Global Variables ===
node_stats_t nodes[MAX_NODES];
uint8_t activeNodes[MAX_NODES];
uint8_t activeNodeCount = 0;
lora_status_t loraStatus = {false, 0, 0, 0, 0, 0, 0.0};

volatile bool receivedFlag = false;
uint8_t rxBuffer[64];

// === Packet Loss Detection ===
#define PACKET_HISTORY_SIZE 32
uint32_t packetHistory[MAX_NODES][PACKET_HISTORY_SIZE];
uint8_t packetHistoryIndex[MAX_NODES];

// === Function Prototypes ===
void tftInit();
void tftShowStatus(const char* line1, const char* line2 = "", const char* line3 = "");
void tftDisplayNode(uint8_t nodeIdx);
void tftDisplaySummary();
bool loraInit();
void setFlag();
bool validateChecksum(lora_payload_t* payload);
void unpackPayload(uint8_t* buffer, lora_payload_t* payload);
int8_t getNodeStatsIndex(uint8_t nodeId);
void updateNodeStats(uint8_t nodeId, lora_payload_t* payload, int rssi, float snr);
bool isDuplicatePacket(uint8_t nodeId, uint32_t packetId);
void detectPacketLoss(uint8_t nodeId, uint32_t currentPacketId);
void loraHandle();
void loraRxBlink();
String formatTime(uint32_t seconds);

// === TFT Functions ===
void tftInit() {
  pinMode(TFT_LED_K, OUTPUT);
  digitalWrite(TFT_LED_K, HIGH);
  
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.print("LoRa Gateway");
  tft.drawFastHLine(0, 10, 160, ST7735_CYAN);
}

void tftShowStatus(const char* line1, const char* line2, const char* line3) {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  
  tft.setCursor(0, 20);
  tft.print(line1);
  if (strlen(line2) > 0) {
    tft.setCursor(0, 35);
    tft.print(line2);
  }
  if (strlen(line3) > 0) {
    tft.setCursor(0, 50);
    tft.print(line3);
  }
}

void tftDisplayNode(uint8_t nodeIdx) {
  if (nodeIdx >= activeNodeCount) return;
  
  node_stats_t* node = &nodes[activeNodes[nodeIdx]];
  if (!node->valid) return;
  
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  
  // Node ID & Status
  tft.setTextColor(node->hitStatus ? ST7735_RED : ST7735_GREEN);
  tft.setTextSize(1);
  tft.setCursor(0, 14);
  tft.printf("Node %d %s", node->nodeId, node->hitStatus ? "[HIT]" : "[OK]");
  
  // IR Address
  if (node->irAddress > 0) {
    tft.setTextColor(ST7735_YELLOW);
    tft.setCursor(0, 26);
    tft.printf("IR: 0x%04X/0x%02X", node->irAddress, node->irCommand);
  }
  
  // GPS
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(0, 38);
  if (node->satellites >= 4) {
    tft.printf("%.5f", node->lat);
    tft.setCursor(0, 48);
    tft.printf("%.5f", node->lng);
  } else {
    tft.print("GPS: No Fix");
  }
  
  // Stats
  tft.setTextColor(ST7735_CYAN);
  tft.setCursor(0, 60);
  tft.printf("RSSI:%d SNR:%.1f", node->lastRSSI, node->lastSNR);
  
  tft.setCursor(0, 70);
  tft.printf("RX:%u Loss:%u", node->rxCount, node->lostPackets);
}

void tftDisplaySummary() {
  tft.fillRect(0, 12, 160, 68, ST7735_BLACK);
  
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 15);
  tft.printf("Nodes: %d/%d", activeNodeCount, MAX_NODES);
  
  tft.setCursor(0, 30);
  tft.printf("RX: %u", loraStatus.rxCount);
  
  tft.setCursor(0, 42);
  tft.printf("Errors: %u", loraStatus.rxErrors);
  
  tft.setCursor(0, 54);
  tft.printf("CRC Err: %u", loraStatus.checksumErrors);
  
  tft.setCursor(0, 66);
  tft.printf("Dupe: %u", loraStatus.duplicates);
}

// === LoRa Functions ===
void setFlag() {
  receivedFlag = true;
}

bool loraInit() {
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  pinMode(LORA_LED_PIN, OUTPUT);
  digitalWrite(LORA_LED_PIN, LOW);

  Serial.print(F("[LoRa] Initializing SX1262... "));
  
  // Reset LoRa module
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(100);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(100);
  
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Failed: %d\n", state);
    return false;
  }

  Serial.println(F("OK"));
  
  // Configure LoRa
  radio.setFrequency(LORA_FREQUENCY);
  radio.setBandwidth(LORA_BANDWIDTH);
  radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
  radio.setCodingRate(LORA_CODING_RATE);
  radio.setSyncWord(LORA_SYNC_WORD);
  radio.setPreambleLength(LORA_PREAMBLE_LEN);
  radio.setCRC(LORA_CRC);
  
  // Set interrupt untuk RX
  radio.setDio1Action(setFlag);
  
  // Start listening
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[LoRa] Start RX failed: %d\n", state);
    return false;
  }

  loraStatus.initialized = true;
  Serial.printf("[LoRa] Gateway Ready - Freq: %.1f MHz, SF: %d\n", 
                LORA_FREQUENCY, LORA_SPREADING_FACTOR);
  
  return true;
}

bool validateChecksum(lora_payload_t* payload) {
  uint8_t* bytes = (uint8_t*)payload;
  uint8_t sum = 0;
  
  for (size_t i = 0; i < sizeof(lora_payload_t) - 1; i++) {
    sum ^= bytes[i];
  }
  
  return (sum == payload->checksum);
}

void unpackPayload(uint8_t* buffer, lora_payload_t* payload) {
  memcpy(payload, buffer, sizeof(lora_payload_t));
}

int8_t getNodeStatsIndex(uint8_t nodeId) {
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodes[i].valid && nodes[i].nodeId == nodeId) {
      return i;
    }
  }
  return -1;
}

bool isDuplicatePacket(uint8_t nodeId, uint32_t packetId) {
  int8_t idx = getNodeStatsIndex(nodeId);
  if (idx < 0) return false;
  
  // Check dalam history
  for (int i = 0; i < PACKET_HISTORY_SIZE; i++) {
    if (packetHistory[idx][i] == packetId) {
      return true;
    }
  }
  
  return false;
}

void detectPacketLoss(uint8_t nodeId, uint32_t currentPacketId) {
  int8_t idx = getNodeStatsIndex(nodeId);
  if (idx < 0) return;
  
  uint32_t lastId = nodes[idx].lastPacketId;
  
  if (lastId > 0 && currentPacketId > lastId + 1) {
    uint32_t lost = currentPacketId - lastId - 1;
    nodes[idx].lostPackets += lost;
    
    Serial.printf("[Node %d] Packet loss detected! Lost %u packets (%u -> %u)\n",
                  nodeId, lost, lastId, currentPacketId);
  }
  
  nodes[idx].lastPacketId = currentPacketId;
  
  // Simpan ke history untuk duplicate detection
  uint8_t histIdx = packetHistoryIndex[idx];
  packetHistory[idx][histIdx] = currentPacketId;
  packetHistoryIndex[idx] = (histIdx + 1) % PACKET_HISTORY_SIZE;
}

void updateNodeStats(uint8_t nodeId, lora_payload_t* payload, int rssi, float snr) {
  int8_t idx = getNodeStatsIndex(nodeId);
  
  if (idx >= 0) {
    // Update existing node
    nodes[idx].lastSeen = millis() / 1000;
    nodes[idx].lastRSSI = rssi;
    nodes[idx].lastSNR = snr;
    nodes[idx].lastBattery = payload->battery;
    nodes[idx].lat = payload->lat / 1000000.0;
    nodes[idx].lng = payload->lng / 1000000.0;
    nodes[idx].alt = payload->alt;
    nodes[idx].satellites = payload->satellites;
    nodes[idx].irAddress = payload->irAddress;
    nodes[idx].irCommand = payload->irCommand;
    nodes[idx].hitStatus = payload->hitStatus;
    nodes[idx].rxCount++;
    
  } else {
    // Add new node
    for (int i = 0; i < MAX_NODES; i++) {
      if (!nodes[i].valid) {
        nodes[i].nodeId = nodeId;
        nodes[i].valid = true;
        nodes[i].rxCount = 1;
        nodes[i].lostPackets = 0;
        nodes[i].lastPacketId = 0;
        nodes[i].lastRSSI = rssi;
        nodes[i].lastSNR = snr;
        nodes[i].lastBattery = payload->battery;
        nodes[i].lastSeen = millis() / 1000;
        nodes[i].lat = payload->lat / 1000000.0;
        nodes[i].lng = payload->lng / 1000000.0;
        nodes[i].alt = payload->alt;
        nodes[i].satellites = payload->satellites;
        nodes[i].irAddress = payload->irAddress;
        nodes[i].irCommand = payload->irCommand;
        nodes[i].hitStatus = payload->hitStatus;
        
        activeNodes[activeNodeCount++] = i;
        
        Serial.printf("[Gateway] New node registered: Node %d\n", nodeId);
        break;
      }
    }
  }
}

void loraRxBlink() {
  digitalWrite(LORA_LED_PIN, HIGH);
  delay(50);
  digitalWrite(LORA_LED_PIN, LOW);
}

void loraHandle() {
  if (!loraStatus.initialized) return;
  
  if (receivedFlag) {
    receivedFlag = false;
    
    int state = radio.readData(rxBuffer, sizeof(lora_payload_t));
    
    if (state == RADIOLIB_ERR_NONE) {
      lora_payload_t payload;
      unpackPayload(rxBuffer, &payload);
      
      // Validate header & group
      if (payload.header != 0xAA || payload.groupId != GROUP_ID) {
        loraStatus.rxErrors++;
        Serial.println("[LoRa] Invalid header/group");
        radio.startReceive();
        return;
      }
      
      // Validate checksum
      if (!validateChecksum(&payload)) {
        loraStatus.checksumErrors++;
        Serial.printf("[LoRa] Checksum error from Node %d\n", payload.nodeId);
        radio.startReceive();
        return;
      }
      
      // Check duplicate menggunakan uptime sebagai packet ID
      if (isDuplicatePacket(payload.nodeId, payload.uptime)) {
        loraStatus.duplicates++;
        Serial.printf("[LoRa] Duplicate packet from Node %d\n", payload.nodeId);
        radio.startReceive();
        return;
      }
      
      // Detect packet loss
      detectPacketLoss(payload.nodeId, payload.uptime);
      
      // Get RSSI & SNR
      int rssi = radio.getRSSI();
      float snr = radio.getSNR();
      
      // Update node stats
      updateNodeStats(payload.nodeId, &payload, rssi, snr);
      
      loraStatus.rxCount++;
      loraStatus.lastRSSI = rssi;
      loraStatus.lastSNR = snr;
      
      loraRxBlink();
      
      // Log received data
      Serial.printf("[RX Node %d] IR:0x%04X GPS:%.6f,%.6f Alt:%dm Sats:%d HIT:%d RSSI:%d SNR:%.1f\n",
                    payload.nodeId, payload.irAddress,
                    payload.lat / 1000000.0, payload.lng / 1000000.0,
                    payload.alt, payload.satellites, payload.hitStatus,
                    rssi, snr);
      
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
      loraStatus.rxErrors++;
      Serial.printf("[LoRa] RX error: %d\n", state);
    }
    
    // Restart receive
    radio.startReceive();
  }
}

String formatTime(uint32_t seconds) {
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds % 3600) / 60;
  uint32_t secs = seconds % 60;
  
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, secs);
  return String(buffer);
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println(F("\n=== LoRa Gateway - Heltec Tracker V1.2 ==="));

  // Enable VEXT
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, HIGH);
  delay(100);

  // Init TFT
  tftInit();
  tftShowStatus("Initializing...", "LoRa Gateway");

  // Init node array
  for (int i = 0; i < MAX_NODES; i++) {
    nodes[i].valid = false;
    nodes[i].rxCount = 0;
    nodes[i].lostPackets = 0;
    nodes[i].lastPacketId = 0;
    packetHistoryIndex[i] = 0;
    
    for (int j = 0; j < PACKET_HISTORY_SIZE; j++) {
      packetHistory[i][j] = 0;
    }
  }

  // Init LoRa
  if (!loraInit()) {
    tftShowStatus("ERROR", "LoRa Init Failed", "Check Serial");
    while (1) {
      digitalWrite(LORA_LED_PIN, HIGH);
      delay(200);
      digitalWrite(LORA_LED_PIN, LOW);
      delay(200);
    }
  }

  tftShowStatus("Gateway Ready", "Waiting data...");
  Serial.println(F("[System] Gateway ready - listening for nodes\n"));
}

// === Main Loop ===
void loop() {
  static unsigned long lastDisplay = 0;
  static uint8_t displayMode = 0;  // 0=summary, 1-n=nodes
  
  // Handle LoRa RX
  loraHandle();
  
  // Cycle display setiap DISPLAY_CYCLE_MS
  if (millis() - lastDisplay >= DISPLAY_CYCLE_MS) {
    lastDisplay = millis();
    
    if (activeNodeCount == 0) {
      tftDisplaySummary();
    } else {
      if (displayMode == 0) {
        tftDisplaySummary();
        displayMode = 1;
      } else {
        tftDisplayNode(displayMode - 1);
        displayMode++;
        if (displayMode > activeNodeCount) {
          displayMode = 0;
        }
      }
    }
  }
  
  // Check node timeout
  uint32_t currentTime = millis() / 1000;
  for (int i = 0; i < activeNodeCount; i++) {
    node_stats_t* node = &nodes[activeNodes[i]];
    if (node->valid && (currentTime - node->lastSeen > NODE_TIMEOUT_SEC)) {
      Serial.printf("[Gateway] Node %d timeout (last seen: %s ago)\n",
                    node->nodeId, formatTime(currentTime - node->lastSeen).c_str());
    }
  }
  
  delay(10);
}