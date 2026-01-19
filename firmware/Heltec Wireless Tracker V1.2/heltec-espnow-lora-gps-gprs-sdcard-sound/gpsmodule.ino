#include "config.h"

void gpsTask(void *pv) {
  for (;;) {
    while (Serial1.available() > 0) {
      GPS.encode(Serial1.read());
    }

    bool hasTime = GPS.time.isValid();
    bool hasLoc = GPS.location.isValid();

    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
      g_gpsData.valid = hasTime || hasLoc;
      if (hasLoc) {
        g_gpsData.lat = GPS.location.lat();
        g_gpsData.lng = GPS.location.lng();
        g_gpsData.alt = GPS.altitude.meters();
      } else {
        g_gpsData.lat = g_gpsData.lng = g_gpsData.alt = 0.0f;
      }
      g_gpsData.satellites = GPS.satellites.value();
      if (hasTime) {
        g_gpsData.hour = GPS.time.hour();
        g_gpsData.minute = GPS.time.minute();
        g_gpsData.second = GPS.time.second();
      } else {
        g_gpsData.hour = g_gpsData.minute = g_gpsData.second = 0;
      }
      xSemaphoreGive(xGpsMutex);
    }

    // Update TFT
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
      g_TftPageData[1].rows[0] = "GPS Status";
      g_TftPageData[1].rows[1] = hasLoc ? ("Lat: " + String(g_gpsData.lat, 6)) : "Lat: -";
      g_TftPageData[1].rows[2] = hasLoc ? ("Long: " + String(g_gpsData.lng, 6)) : "Long: -";
      g_TftPageData[1].rows[3] = hasLoc ? ("Alt: " + String(g_gpsData.alt, 1) + " m") : "Alt: -";
      g_TftPageData[1].rows[4] = "Sat: " + String(g_gpsData.satellites);

      int8_t displayHour = 0;
      if (hasTime) {
        displayHour = g_gpsData.hour + TIMEZONE_OFFSET_HOURS;
        if (displayHour >= 24) displayHour -= 24;
        else if (displayHour < 0) displayHour += 24;
      }
      char timeBuf[12];
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
               hasTime ? displayHour : 0,
               hasTime ? g_gpsData.minute : 0,
               hasTime ? g_gpsData.second : 0);
      g_TftPageData[1].rows[5] = "Time: " + String(timeBuf);
      xSemaphoreGive(xTftMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}