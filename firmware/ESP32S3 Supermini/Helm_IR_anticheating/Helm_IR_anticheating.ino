/*
 * Penerima IR (Protokol NEC) dengan Anti-Cheating
 * Library: IRremote by Armin Joachimsmeyer (https://github.com/Arduino-IRremote/Arduino-IRremote)
 */
#include <Arduino.h>
#include <IRremote.hpp>

// Configuration
#define IR_RECEIVE_PIN 4          // Pin connected to IR receiver (e.g., VS1838B)
#define IR_LED_PIN 6              // Pin untuk Infrared LED anti-cheating
#define PHOTODIODE_PIN 5          // Pin input dari M74HC4078 (NOR gate output)
#define ENABLE_LED_FEEDBACK false // Optional LED feedback for received signals

// Anti-cheating variables
bool antiCheatActive = true;      // Status anti-cheat system
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 100; // Check setiap 100ms

void setup() {
  Serial.begin(115200);
  
  // Setup anti-cheating pins
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(PHOTODIODE_PIN, INPUT);
  
  // Nyalakan IR LED untuk anti-cheating
  digitalWrite(IR_LED_PIN, HIGH);
  
  // Start the IR receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.println("IR Receiver Ready");
  Serial.println("Anti-Cheating System Active");
  Serial.println("Waiting for NEC IR signals...");
}

void loop() {
  // Check anti-cheating system periodically
  if (millis() - lastCheckTime >= CHECK_INTERVAL) {
    lastCheckTime = millis();
    checkAntiCheat();
  }
  
  // Check if IR signal is received
  if (IrReceiver.decode()) {
    
    // Hanya proses signal jika anti-cheat tidak terdeteksi
    if (antiCheatActive) {
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
    } else {
      Serial.println("⚠️ SIGNAL REJECTED - Anti-Cheat Triggered!");
    }
    
    // Resume receiving
    IrReceiver.resume(); // Enable receiving of the next IR signal
  }
}

void checkAntiCheat() {
  // Baca status photodiode dari NOR gate
  // Logika NOR: Output HIGH ketika semua photodiode TIDAK menerima IR (normal)
  // Output LOW ketika ada photodiode yang ditutup/terhalang
  int photodiodeStatus = digitalRead(PHOTODIODE_PIN);
  
  if (photodiodeStatus == LOW) {// LED BIASA LOW KALAU IR HIGH
    // Anti-cheat terdeteksi! (IR tertutup)
    if (antiCheatActive) {
      antiCheatActive = false;
      Serial.println("  WARNING: CHEATING DETECTED!");
      Serial.println("  Sinyal IR tidak akan diproses");
    }
  } else {
    // Kondisi normal
    if (!antiCheatActive) {
      antiCheatActive = true;
      Serial.println("✓ Anti-Cheat System Normal - Signals Accepted");
    }
  }
}