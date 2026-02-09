#include <Arduino.h>
#include <TinyGPS++.h>

#define Vext_Ctrl 3
#define GPS_RX 33
#define GPS_TX 34

TinyGPSPlus gps;

void setup() {
  // Power on GPS
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  
  delay(500);
  
  Serial.begin(115200);
  
  Serial.println("\n\n=== GPS DEBUG TEST ===");
  Serial.println("Testing different baud rates...\n");
  
  // Test berbagai baud rate
  int baudRates[] = {4800, 9600, 38400, 57600, 115200};
  
  for (int i = 0; i < 5; i++) {
    Serial.printf("\n--- Testing %d baud ---\n", baudRates[i]);
    Serial1.begin(baudRates[i], SERIAL_8N1, GPS_RX, GPS_TX);
    
    delay(2000); // Tunggu 2 detik
    
    int charCount = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < 3000) { // Test selama 3 detik
      while (Serial1.available()) {
        char c = Serial1.read();
        Serial.print(c); // Print raw data
        charCount++;
      }
    }
    
    Serial.printf("\nChars received: %d\n", charCount);
    Serial1.end();
    delay(500);
  }
  
  Serial.println("\n\n=== Test selesai ===");
  Serial.println("Lihat baud rate mana yang menampilkan data NMEA");
  Serial.println("Data NMEA biasanya dimulai dengan $GPGGA, $GPRMC, dll");
}

void loop() {
  // Empty
}
