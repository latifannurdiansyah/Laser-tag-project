#ifndef IRHANDLER_HPP
#define IRHANDLER_HPP

#include <Arduino.h>
#include <IRremote.hpp>

class IRHandler {
public:
  static void begin(uint8_t pin);
  static bool available();
  static void resume();
  static uint16_t getAddress();
  static uint8_t getCommand();
  static uint16_t getNumberOfBits();
};

#endif