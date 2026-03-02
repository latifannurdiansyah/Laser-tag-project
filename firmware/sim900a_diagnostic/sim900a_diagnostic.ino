/*
 * ============================================================
 *  SIM900A DIAGNOSTIC TEST
 *  Heltec Wireless Tracker v1.2
 *  TX ESP32 = GPIO17 --> RX SIM900A
 *  RX ESP32 = GPIO18 --> TX SIM900A
 * ============================================================
 *  Buka Serial Monitor @ 115200 baud
 *  Program ini mencoba berbagai baud rate secara otomatis
 *  dan mencetak RAW response dari SIM900A
 * ============================================================
 */

#include <HardwareSerial.h>

#define SIM_TX      17
#define SIM_RX      18
#define SERIAL_BAUD 115200

HardwareSerial simSerial(1);

// Daftar baud rate yang akan dicoba
long baudRates[] = {9600, 19200, 38400, 57600, 115200};
int  numBauds    = 5;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(2000);

  Serial.println("============================================");
  Serial.println("  SIM900A DIAGNOSTIC - Heltec WT v1.2");
  Serial.println("============================================");
  Serial.println("Memulai deteksi baud rate otomatis...\n");

  for (int b = 0; b < numBauds; b++) {
    long br = baudRates[b];
    Serial.printf(">>> Mencoba baud rate: %ld\n", br);

    simSerial.begin(br, SERIAL_8N1, SIM_RX, SIM_TX);
    delay(500);

    // Flush buffer
    while (simSerial.available()) simSerial.read();

    // Kirim AT
    simSerial.println("AT");
    delay(1000);

    String resp = "";
    unsigned long t = millis();
    while (millis() - t < 2000) {
      while (simSerial.available()) {
        char c = simSerial.read();
        resp += c;
      }
      if (resp.indexOf("OK") >= 0) break;
      delay(10);
    }

    resp.trim();
    Serial.printf("    RAW response: [%s]\n", resp.c_str());

    if (resp.indexOf("OK") >= 0) {
      Serial.printf("\n[SUCCESS] SIM900A ditemukan di baud rate: %ld\n", br);
      Serial.println("============================================");
      Serial.println("Menjalankan tes AT commands...\n");
      runTests();
      return;
    } else {
      Serial.println("    Tidak ada respons.\n");
    }

    simSerial.end();
    delay(200);
  }

  Serial.println("============================================");
  Serial.println("[FAIL] Tidak ada respons dari SIM900A!");
  Serial.println("Periksa:");
  Serial.println("  1. Power supply >= 2A");
  Serial.println("  2. Kabel TX/RX (coba tukar)");
  Serial.println("  3. GND terhubung ke ESP32");
  Serial.println("  4. Modul SIM900A menyala (LED berkedip)");
  Serial.println("============================================");
}

void runTests() {
  // Test 1: Matikan echo
  sendAndPrint("ATE0");
  delay(500);

  // Test 2: Info modul
  sendAndPrint("ATI");
  delay(500);

  // Test 3: Cek SIM
  sendAndPrint("AT+CPIN?");
  delay(500);

  // Test 4: Kualitas sinyal
  sendAndPrint("AT+CSQ");
  delay(500);

  // Test 5: Registrasi jaringan
  sendAndPrint("AT+CREG?");
  delay(500);

  // Test 6: Operator jaringan
  sendAndPrint("AT+COPS?");
  delay(500);

  Serial.println("\n============================================");
  Serial.println("Diagnosa selesai. Masukkan AT command manual");
  Serial.println("di Serial Monitor untuk tes lanjutan.");
  Serial.println("============================================");
}

void sendAndPrint(String cmd) {
  Serial.println(">> " + cmd);
  simSerial.println(cmd);

  String resp = "";
  unsigned long t = millis();
  while (millis() - t < 3000) {
    while (simSerial.available()) {
      char c = simSerial.read();
      resp += c;
    }
    if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) break;
    delay(10);
  }
  resp.trim();
  Serial.println("<< " + resp);
  Serial.println();
}

// Loop: relay Serial <-> SIM900A untuk tes manual
void loop() {
  // Kirim dari Serial Monitor ke SIM900A
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.println(">> " + input);
    simSerial.println(input);
  }

  // Tampilkan respons SIM900A ke Serial Monitor
  while (simSerial.available()) {
    char c = simSerial.read();
    Serial.print(c);
  }
}
