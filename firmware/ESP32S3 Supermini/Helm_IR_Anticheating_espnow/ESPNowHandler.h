#ifndef ESPNOWHANDLER_H
#define ESPNOWHANDLER_H

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
  static void begin(const uint8_t* peerMac) {
    memcpy(_peerMac, peerMac, 6);
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    WiFi.setSleep(false);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_now_init();
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _peerMac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }

  static void sendPacket(uint16_t address, uint8_t command) {
    _lastPacket.address = address;
    _lastPacket.command = command;
    _dataPending = true;
    _sendOK = false;
    esp_err_t result = esp_now_send(_peerMac, (uint8_t*)&_lastPacket, sizeof(_lastPacket));
    if (result == ESP_OK) delay(10);
  }

  static void retryIfNeeded() {
    if (_dataPending && !_sendOK) {
      esp_err_t result = esp_now_send(_peerMac, (uint8_t*)&_lastPacket, sizeof(_lastPacket));
      if (result == ESP_OK) {
        delay(10);
        if (_sendOK) _dataPending = false;
      }
    }
  }

private:
  static ir_data_t _lastPacket;
  static bool _dataPending;
  static volatile bool _sendOK;
  static uint8_t _peerMac[6];

  static void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    _sendOK = (status == ESP_NOW_SEND_SUCCESS);
  }
};

// Definisi static member
ir_data_t ESPNowHandler::_lastPacket = {0, 0};
bool ESPNowHandler::_dataPending = false;
volatile bool ESPNowHandler::_sendOK = false;
uint8_t ESPNowHandler::_peerMac[6];

#endif