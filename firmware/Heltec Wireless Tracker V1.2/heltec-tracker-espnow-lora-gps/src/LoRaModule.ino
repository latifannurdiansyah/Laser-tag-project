#include "PinConfig.h"

SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
uint32_t txCount = 0;
uint32_t txErrors = 0;

bool loraInit() {
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  pinMode(LORA_LED_PIN, OUTPUT);
  digitalWrite(LORA_LED_PIN, LOW);

  Serial.print(F("[LoRa] Initializing SX1262... "));
  
  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(100);
  digitalWrite(RADIO_RST_PIN, HIGH);
  delay(100);
  
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Failed, code: ")); Serial.println(state);
    return false;
  }

  if ((radio.setFrequency(LORA_FREQUENCY) != RADIOLIB_ERR_NONE) ||
      (radio.setBandwidth(LORA_BANDWIDTH) != RADIOLIB_ERR_NONE) ||
      (radio.setSpreadingFactor(LORA_SPREADING_FACTOR) != RADIOLIB_ERR_NONE) ||
      (radio.setCodingRate(LORA_CODING_RATE) != RADIOLIB_ERR_NONE) ||
      (radio.setSyncWord(LORA_SYNC_WORD) != RADIOLIB_ERR_NONE) ||
      (radio.setOutputPower(LORA_TX_POWER) != RADIOLIB_ERR_NONE) ||
      (radio.setPreambleLength(LORA_PREAMBLE_LEN) != RADIOLIB_ERR_NONE) ||
      (radio.setCRC(LORA_CRC) != RADIOLIB_ERR_NONE)) {
    Serial.println(F("[LoRa] Config failed"));
    return false;
  }

  Serial.println(F("OK"));
  Serial.printf("[LoRa] Freq: %.1f MHz, SF: %d, BW: %.1f kHz\n", 
                LORA_FREQUENCY, LORA_SPREADING_FACTOR, LORA_BANDWIDTH);
  return true;
}

void packPayload(lora_payload_t* payload) {
  payload->header = 0xAA;
  payload->groupId = GROUP_ID;
  payload->nodeId = NODE_ID;
  payload->lat = (int32_t)(currentLat * 1000000);
  payload->lng = (int32_t)(currentLon * 1000000);
  payload->alt = (int16_t)(currentAlt);
  payload->satellites = currentSats;
  payload->battery = readBatteryVoltage();
  payload->uptime = millis() / 1000;
  payload->irAddress = latestIr.address;
  payload->irCommand = latestIr.command;
  payload->hitStatus = (latestIr.address > 0) ? 1 : 0;

  uint8_t* bytes = (uint8_t*)payload;
  uint8_t sum = 0;
  for (size_t i = 0; i < sizeof(lora_payload_t) - 1; i++) {
    sum ^= bytes[i];
  }
  payload->checksum = sum;
}

void loraSendData() {
  lora_payload_t payload;
  packPayload(&payload);
  
  int state = radio.transmit((uint8_t*)&payload, sizeof(lora_payload_t));
  
  if (state == RADIOLIB_ERR_NONE) {
    txCount++;
    loraTxBlink();
    Serial.printf("[LoRa TX #%u] IR:0x%04X GPS:%.6f,%.6f HIT:%d\n",
                  txCount, payload.irAddress, currentLat, currentLon, payload.hitStatus);
  } else {
    txErrors++;
    Serial.printf("[LoRa TX] Error: %d\n", state);
  }
}

void loraTxBlink() {
  digitalWrite(LORA_LED_PIN, HIGH);
  delay(50);
  digitalWrite(LORA_LED_PIN, LOW);
}