#include "AntiCheat.hpp"

bool AntiCheat::_active = true;
unsigned long AntiCheat::_lastCheck = 0;
const unsigned long AntiCheat::CHECK_INTERVAL = 100; // ms
uint8_t AntiCheat::_sensorPin = 0;
uint8_t AntiCheat::_irLedPin = 0;

void AntiCheat::begin(uint8_t irLedPin, uint8_t sensorPin) {
  _irLedPin = irLedPin;
  _sensorPin = sensorPin;
  pinMode(_irLedPin, OUTPUT);
  pinMode(_sensorPin, INPUT);
  digitalWrite(_irLedPin, HIGH); // Nyalakan IR LED terus-menerus
}

void AntiCheat::update() {
  if (millis() - _lastCheck >= CHECK_INTERVAL) {
    _lastCheck = millis();
    int status = digitalRead(_sensorPin); // LOW = cheating
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

bool AntiCheat::isActive() {
  return _active;
}