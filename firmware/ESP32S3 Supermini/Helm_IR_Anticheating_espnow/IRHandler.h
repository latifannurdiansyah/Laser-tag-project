#ifndef IRHANDLER_H
#define IRHANDLER_H

class IRHandler {
public:
  static void begin(uint8_t pin) {
    IrReceiver.begin(pin, false);
  }

  static bool available() {
    return IrReceiver.decode();
  }

  static void resume() {
    IrReceiver.resume();
  }

  static uint16_t getAddress() {
    return IrReceiver.decodedIRData.address;
  }

  static uint8_t getCommand() {
    return IrReceiver.decodedIRData.command;
  }

  static uint16_t getNumberOfBits() {
    return IrReceiver.decodedIRData.numberOfBits;
  }

  static uint32_t getDecodedRawData() {
    return (uint32_t)IrReceiver.decodedIRData.decodedRawData;
  }

  static unsigned long getInitialGapMicros() {
    return IrReceiver.irparams.initialGapTicks * MICROS_PER_TICK;
  }

  static unsigned long getDurationMicros() {
    return IrReceiver.decodedIRData.rawlen * MICROS_PER_TICK;
  }
};

#endif