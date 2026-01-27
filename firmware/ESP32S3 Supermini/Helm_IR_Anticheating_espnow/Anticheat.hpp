#ifndef ANTICHEAT_HPP
#define ANTICHEAT_HPP

#include <Arduino.h>

class AntiCheat {
public:
  static void begin(uint8_t irLedPin, uint8_t sensorPin);
  static void update();
  static bool isActive();

private:
  static bool _active;
  static unsigned long _lastCheck;
  static const unsigned long CHECK_INTERVAL;
  static uint8_t _sensorPin;
  static uint8_t _irLedPin;
};

#endif