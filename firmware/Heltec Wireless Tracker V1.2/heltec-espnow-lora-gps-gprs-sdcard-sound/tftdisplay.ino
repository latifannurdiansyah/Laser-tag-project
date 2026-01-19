#include "config.h"

void tftTask(void *pv) {
  uint32_t pageSwitch = millis();
  uint8_t currentPage = 0;

  for (;;) {
    if (millis() - pageSwitch >= PAGE_SWITCH_MS) {
      currentPage = (currentPage + 1) % TFT_MAX_PAGES;
      pageSwitch = millis();
    }

    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
      framebuffer.fillScreen(ST7735_BLACK);
      framebuffer.setTextColor(ST7735_WHITE);
      framebuffer.setTextSize(1);
      for (int i = 0; i < TFT_ROWS_PER_PAGE; i++) {
        int y = TFT_TOP_MARGIN + i * TFT_LINE_HEIGHT;
        if (y >= 80) break;
        framebuffer.setCursor(TFT_LEFT_MARGIN, y);
        framebuffer.print(g_TftPageData[currentPage].rows[i]);
      }
      xSemaphoreGive(xTftMutex);
      tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), TFT_WIDTH, TFT_HEIGHT);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}