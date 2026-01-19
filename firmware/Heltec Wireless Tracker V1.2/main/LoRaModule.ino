#include "PinConfig.h"

SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
String loraWanStatus = "Joining";

bool loraWanInit() {
  loraWanStatus = "Joining";

  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    loraWanStatus = "RadioFail";
    return false;
  }

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state != RADIOLIB_ERR_NONE) {
    loraWanStatus = "JoinFailed";
    return false;
  }

  state = node.activateOTAA();
  if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
    loraWanStatus = "Joined";
    return true;
  } else {
    loraWanStatus = "JoinFailed";
    return false;
  }
}

void loraWanSendData() {
  ttn_payload_t payload;
  payload.irAddress = latestIr.address;
  payload.irCommand = latestIr.command;
  payload.lat = (int32_t)(currentLat * 1000000);
  payload.lng = (int32_t)(currentLon * 1000000);
  payload.alt = (int16_t)(currentAlt);
  payload.satellites = currentSats;
  payload.hitStatus = (latestIr.address > 0) ? 1 : 0;

  if (timeValid) {
    int8_t h = currentHour + TIMEZONE_OFFSET_HOURS;
    if (h >= 24) h -= 24; else if (h < 0) h += 24;
    payload.hour = (uint8_t)h;
  } else {
    payload.hour = 0;
  }
  payload.minute = currentMinute;
  payload.second = currentSecond;

  int state = node.sendReceive((uint8_t*)&payload, sizeof(payload));
  if (state >= RADIOLIB_ERR_NONE) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.printf("[TTN TX] OK | IR:0x%04X\n", payload.irAddress);
  } else {
    Serial.printf("[TTN TX] Err: %d\n", state);
  }
}