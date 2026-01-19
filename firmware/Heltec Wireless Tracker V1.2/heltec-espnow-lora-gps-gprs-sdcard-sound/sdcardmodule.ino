#include "config.h"

void sdCardTask(void *pv) {
  if (!SD.begin(SD_CS, SPI, 8000000)) {
    Serial.println("SD Mount Failed!");
    vTaskDelete(NULL);
  }

  if (SD.exists("/tracker.log")) {
    SD.remove("/tracker.log");
    Serial.println("Old tracker.log deleted.");
  }

  File file;
  uint32_t lastWrite = 0;

  for (;;) {
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

void logToSd(const String &msg) {
  String *copy = new String(msg);
  if (xQueueSendToBack(xLogQueue, &copy, pdMS_TO_TICKS(10)) != pdTRUE) {
    delete copy;
  }
}

String formatLogLine() {
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