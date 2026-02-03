#ifndef ANTICHEAT_H
#define ANTICHEAT_H

#include <Arduino.h>

class AntiCheat {
public:
  static void begin(uint8_t irLedPin, uint8_t sensorPin) {
    _irLedPin = irLedPin;
    _sensorPin = sensorPin;
    pinMode(_irLedPin, OUTPUT);
    pinMode(_sensorPin, INPUT);
    digitalWrite(_irLedPin, HIGH);
  }

  static void update() {
    if (millis() - _lastCheck >= CHECK_INTERVAL) {
      _lastCheck = millis();
      int status = digitalRead(_sensorPin);
      if (status == LOW) {
        if (_active) {
          _active = false;
          Serial.println("  WARNING: CHEATING DETECTED!");
          Serial.println("  Sinyal IR tidak akan diproses");
        }
      } else {
        if (!_active) {
          _active = true;
          Serial.println("✓ Anti-Cheat System Normal - Signals Accepted");
        }
      }
    }
  }

  static bool isActive() {
    return _active;
  }

private:
  static bool _active;
  static unsigned long _lastCheck;
  static const unsigned long CHECK_INTERVAL;
  static uint8_t _sensorPin;
  static uint8_t _irLedPin;
};

// Definisi static member
bool AntiCheat::_active = true;
unsigned long AntiCheat::_lastCheck = 0;
const unsigned long AntiCheat::CHECK_INTERVAL = 100; // ms
uint8_t AntiCheat::_sensorPin = 0;
uint8_t AntiCheat::_irLedPin = 0;

#endif