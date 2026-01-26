/*
 * Penerima IR (Protokol NEC) - Versi Stabil untuk ESP32-S3
 * + Anti-Cheating System dengan LED
 * Library: IRremote by Armin Joachimsmeyer
 */

#include <Arduino.h>
#include <IRremote.hpp>

// === Konfigurasi IR ===
#define IR_RECEIVE_PIN 4
#define ENABLE_LED_FEEDBACK false

// === Anti-Cheating Configuration ===
#define LED_EMITTER_PIN 6     // Pin untuk LED emitter
#define PHOTO_DETECTOR_PIN 5  // Pin untuk output M74HC4078 (photodiode detector)

// === Anti-Cheating Variables ===
bool isCheatingDetected = false;
bool lastCheatingState = false;
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 100; // Check setiap 100ms

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Inisialisasi IR Receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR Receiver Ready");
  Serial.println("Waiting for NEC IR signals...");

  // Setup Anti-Cheating System
  pinMode(LED_EMITTER_PIN, OUTPUT);
  pinMode(PHOTO_DETECTOR_PIN, INPUT_PULLUP); // Tambah PULLUP untuk stabilitas
  
  // Nyalakan LED emitter dengan maksimal
  digitalWrite(LED_EMITTER_PIN, HIGH);
  
  Serial.println("========================================");
  Serial.println("Anti-Cheating System Active");
  Serial.println("LED Emitter: Pin 6 (ON)");
  Serial.println("Photodiode Detector: Pin 5");
  
  // Debug: cek status awal
  delay(500); // Beri waktu LED menyala
  int initialState = digitalRead(PHOTO_DETECTOR_PIN);
  Serial.print("Initial Detector State: ");
  Serial.println(initialState == HIGH ? "HIGH (Normal)" : "LOW (Cheating)");
  Serial.println("========================================");
}

void loop() {
  // Check Anti-Cheating System
  checkAntiCheating();
  
  // Terima sinyal IR hanya jika tidak ada cheating
  if (!isCheatingDetected && IrReceiver.decode()) {
    Serial.println("IR signal received!");
    IrReceiver.printIRResultShort(&Serial);

    if (IrReceiver.decodedIRData.protocol == NEC) {
      Serial.print("Received NEC Signal | Address: 0x");
      Serial.print(IrReceiver.decodedIRData.address, HEX);
      Serial.print(" | Command: 0x");
      Serial.println(IrReceiver.decodedIRData.command, HEX);
      IrReceiver.printIRSendUsage(&Serial);
    } else {
      Serial.print("Received unknown protocol: ");
      Serial.println(IrReceiver.decodedIRData.protocol);
    }

    IrReceiver.resume();
  }
  else if (isCheatingDetected && IrReceiver.decode()) {
    // Buang sinyal jika ada cheating
    Serial.println("Signal IGNORED - Anti-cheating triggered!");
    IrReceiver.resume();
  }

  delay(5);
}

void checkAntiCheating() {
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTime < CHECK_INTERVAL) {
    return;
  }
  lastCheckTime = currentTime;
  
  // Baca status photodiode detector
  int detectorState = digitalRead(PHOTO_DETECTOR_PIN);
  
  // Debug setiap 2 detik
  static unsigned long lastDebug = 0;
  if (currentTime - lastDebug > 2000) {
    lastDebug = currentTime;
    Serial.print("DEBUG - Detector Pin 5 State: ");
    Serial.println(detectorState == HIGH ? "HIGH" : "LOW");
  }
  
  // LOW = LED terhalang/tertutup (photodiode tidak menerima cahaya) → CHEATING
  // HIGH = LED menyala dan diterima photodiode → NORMAL
  isCheatingDetected = (detectorState == HIGH);
  
  // Print peringatan jika status berubah
  if (isCheatingDetected != lastCheatingState) {
    lastCheatingState = isCheatingDetected;
    
    if (isCheatingDetected) {
      Serial.println("");
      Serial.println("========================================");
      Serial.println("  WARNING: CHEATING DETECTED!");
      Serial.println("  LED terhalang / Photodiode tertutup!");
      Serial.println("  Sinyal IR tidak akan diproses");
      Serial.println("========================================");
      Serial.println("");
    } else {
      Serial.println("");
      Serial.println("========================================");
      Serial.println("  System Normal");
      Serial.println("  LED menyala & diterima photodiode");
      Serial.println("  Ready to receive signals");
      Serial.println("========================================");
      Serial.println("");
    }
  }
}