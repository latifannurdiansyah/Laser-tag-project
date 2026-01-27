#ifndef ESPNOWHANDLER_HPP
#define ESPNOWHANDLER_HPP

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

typedef struct {
  uint16_t address;
  uint8_t  command;
} ir_data_t;

class ESPNowHandler {
public:
  static void begin(const uint8_t* peerMac);
  static void sendPacket(uint16_t address, uint8_t command);
  static void retryIfNeeded();
  static bool isPending();

private:
  static ir_data_t _lastPacket;
  static bool _dataPending;
  static volatile bool _sendOK;
  static uint8_t _peerMac[6];
  static void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status);
};

#endif