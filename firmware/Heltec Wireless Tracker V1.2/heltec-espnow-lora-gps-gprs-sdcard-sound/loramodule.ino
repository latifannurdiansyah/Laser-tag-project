#include "config.h"

void loraTask(void *pv) {
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
  int16_t state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    String err = "Radio init failed: " + stateDecode(state);
    Serial.println(err);
    logToSd(err);
    vTaskDelete(NULL);
  }

  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state != RADIOLIB_ERR_NONE) {
    String err = "LoRaWAN init failed: " + stateDecode(state);
    Serial.println(err);
    logToSd(err);
    vTaskDelete(NULL);
  }

  logToSd("Starting OTAA join...");
  Serial.println("Starting OTAA join...");

  uint8_t attempts = 0;
  while (attempts++ < MAX_JOIN_ATTEMPTS) {
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

  // Periodic uplink
  uint32_t lastUplink = 0;
  for (;;) {
    if (millis() - lastUplink >= UPLINK_INTERVAL_MS) {
      DataPayload payload;
      payload.address_id = 0x12345678;
      payload.sub_address_id = random(0, 256);
      payload.shooter_address_id = random(0x10000000, 0xFFFFFFFF);
      payload.shooter_sub_address_id = random(0, 256);
      payload.status = hitDetected ? 1 : 0;

      if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        payload.lat = g_gpsData.lat;
        payload.lng = g_gpsData.lng;
        payload.alt = g_gpsData.alt;
        xSemaphoreGive(xGpsMutex);
      } else {
        payload.lat = payload.lng = payload.alt = 0.0f;
      }

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
      }

      if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_loraStatus.lastEvent = eventStr;
        g_loraStatus.snr = snr;
        g_loraStatus.rssi = rssi;
        xSemaphoreGive(xLoraMutex);
      }

      if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[0].rows[0] = "LoRaWAN Status";
        g_TftPageData[0].rows[1] = "Event: " + eventStr;
        g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
        g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
        g_TftPageData[0].rows[4] = "Batt: " + String(analogReadMilliVolts(0)) + " mV";
        g_TftPageData[0].rows[5] = "Joined: YES";
        xSemaphoreGive(xTftMutex);
      }

      logToSd(formatLogLine());
      lastUplink = millis();
      hitDetected = false; // Reset after send
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}