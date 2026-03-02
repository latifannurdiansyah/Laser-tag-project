#include <Arduino.h>

// Gunakan Serial1 untuk komunikasi dengan GSM
HardwareSerial gsmSerial(1); 

// Pin Power Control untuk Heltec (Biasanya GPIO 19 atau 45)
#define VEXT_PIN 3 

String nomorTujuan = "+6281515332971"; 

void setup() {
  // 1. Nyalakan Power untuk pin eksternal Heltec
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW); // Pada beberapa versi LOW = ON, pada yang lain HIGH = ON
  delay(1000);
  
  Serial.begin(115200); 
  
  // 2. Coba komunikasi RX di 17, TX di 16
  Serial.println("\n--- Memulai Pencarian Modul GSM ---");
  gsmSerial.begin(9600, SERIAL_8N1, 17, 16); 

  delay(3000);
}

void loop() {
  // Loop ini akan terus mengirim 'AT' setiap 2 detik sampai dijawab 'OK'
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 2000) {
    Serial.println("Mengirim: AT...");
    gsmSerial.println("AT");
    lastTime = millis();
  }

  // Baca balasan dari GSM
  if (gsmSerial.available()) {
    Serial.print("GSM Menjawab: ");
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      Serial.write(c);
    }
    Serial.println();
  }

  // Jika kamu ketik sesuatu di Serial Monitor, kirim ke GSM
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "a") {
      gsmSerial.println("ATD" + nomorTujuan + ";");
      Serial.println("Memerintah: Telpon...");
    } else {
      gsmSerial.println(cmd);
    }
  }
}