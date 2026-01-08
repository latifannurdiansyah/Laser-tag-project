/*
 * Penerima IR (Protokol NEC) - Versi Stabil untuk ESP32-S3
 * Library: IRremote by Armin Joachimsmeyer (https://github.com/Arduino-IRremote/Arduino-IRremote)
 */

#include <Arduino.h>
#include <IRremote.hpp>

// Configuration
#define IR_RECEIVE_PIN 4     // Pin connected to IR receiver (e.g., VS1838B)
#define ENABLE_LED_FEEDBACK false // Optional LED feedback for received signals

void setup() {
  Serial.begin(115200);
  
  // Start the IR receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.println("IR Receiver Ready");
  Serial.println("Waiting for NEC IR signals...");
}

void loop() {
  // Check if IR signal is received
  if (IrReceiver.decode()) {
    
    // Print basic decoded info
    Serial.println("IR signal received!");

    // Print full decoded result (for debugging)
    IrReceiver.printIRResultShort(&Serial);

    // Check if it's an NEC protocol
    if (IrReceiver.decodedIRData.protocol == NEC) {
      Serial.print("Received NEC Signal | Address: 0x");
      Serial.print(IrReceiver.decodedIRData.address, HEX);
      Serial.print(" | Command: 0x");
      Serial.println(IrReceiver.decodedIRData.command, HEX);

      // Optional: print usage format (can be used to re-send)
      IrReceiver.printIRSendUsage(&Serial);
    } 
    else {
      Serial.print("Received unknown protocol: ");
      Serial.println(IrReceiver.decodedIRData.protocol);
    }

    // Resume receiving
    IrReceiver.resume(); // Enable receiving of the next IR signal
  }
}