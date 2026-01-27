#include "IRHandler.hpp"

void IRHandler::begin(uint8_t pin) {
  IrReceiver.begin(pin, false);
}

bool IRHandler::available() {
  return IrReceiver.decode();
}

void IRHandler::resume() {
  IrReceiver.resume();
}

uint16_t IRHandler::getAddress() {
  return IrReceiver.decodedIRData.address;
}

uint8_t IRHandler::getCommand() {
  return IrReceiver.decodedIRData.command;
}

uint16_t IRHandler::getNumberOfBits() {
  return IrReceiver.decodedIRData.numberOfBits;
}