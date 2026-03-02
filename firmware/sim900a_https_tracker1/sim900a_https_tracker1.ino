/*
 * ============================================================
 *  SIM900A + Heltec Wireless Tracker v1.2
 *  Kirim Data GPS (acak) via HTTPS POST
 * ============================================================
 *  PIN KONEKSI (WIRING BENAR):
 *    SIM900A TX    --> GPIO 13 (RX ESP32)
 *    SIM900A RX    --> GPIO 14 (TX ESP32)
 *    SIM900A GND   --> GND Heltec
 *    SIM900A VCC   --> VBAT Heltec (3.7V) atau buck converter 4.0V
 *
 *  PIN YANG TIDAK BOLEH DIPAKAI UNTUK SIM900A:
 *    GPIO 17 = GPS UART TX (onboard UC6580)
 *    GPIO 18 = LED_BUILTIN onboard
 *    GPIO  3 = Vext_Ctrl (power kontrol peripheral)
 *    GPIO 21 = LED_K onboard
 * ============================================================
 */

#include <HardwareSerial.h>

// ---- PIN HELTEC WIRELESS TRACKER v1.2 ----
#define Vext_Ctrl     3   // LOW = aktifkan daya ke peripheral (GPS, dll)
#define LED_K         21  // LED onboard katoda
#define LED_BUILTIN   18  // LED onboard (JANGAN pakai untuk UART!)

// ---- PIN SIM900A (gunakan pin bebas) ----
#define SIM_TX        17  // ESP32 TX --> SIM900A RX
#define SIM_RX        18  // ESP32 RX --> SIM900A TX
#define SIM_BAUD      9600
#define SERIAL_BAUD   115200

// ---- KONFIGURASI APN ----
const char* APN      = "indosatgprs";
const char* APN_USER = "indosat";
const char* APN_PASS = "indosat";

// ---- KONFIGURASI SERVER ----
const char* SERVER_HOST = "laser-tag-project.vercel.app";
const int   SERVER_PORT = 443;
const char* SERVER_PATH = "/api/tracker";

// ---- VARIABEL ----
HardwareSerial simSerial(1);  // UART1

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  // Aktifkan Vext agar peripheral onboard mendapat daya
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, LOW);  // LOW = ON untuk Heltec WT
  delay(500);

  // Setup LED onboard
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, LOW);

  Serial.println("==============================================");
  Serial.println("  SIM900A HTTPS Tracker - Heltec WT v1.2");
  Serial.println("==============================================");
  Serial.println("  SIM TX: GPIO14 | SIM RX: GPIO13");
  Serial.println("  Vext_Ctrl: GPIO3 (LOW=ON)");
  Serial.println("==============================================");

  simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(2000);

  Serial.println("[INFO] Inisialisasi SIM900A...");
  blinkLED(2);

  if (!sendAT("AT", "OK", 3000)) {
    Serial.println("[ERROR] SIM900A tidak merespons! Cek wiring & power.");
    while (true) {
      blinkLED(5);
      delay(2000);
    }
  }

  sendAT("ATE0",     "OK",    2000);
  sendAT("AT+CPIN?", "READY", 5000);
  waitForNetwork();
  sendAT("AT+CSQ",   "OK",    2000);

  Serial.println("[INFO] Inisialisasi selesai!");
  Serial.println("==============================================");
  blinkLED(3);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  float  lat        = -6.2000 + (random(-1000, 1000) / 10000.0);
  float  lon        = 106.8000 + (random(-1000, 1000) / 10000.0);
  float  alt        = 10.0 + random(0, 100);
  int    sat        = random(4, 15);
  float  batt       = 3.5 + (random(0, 12) / 10.0);
  int    rssi       = -(random(50, 110));
  float  snr        = random(-5, 15) + (random(0, 9) / 10.0);
  String status_str = (batt > 3.7) ? "OK" : "LOW_BATT";

  Serial.println("----------------------------------------------");
  Serial.println("[DATA] Payload yang akan dikirim:");
  Serial.printf("  LATITUDE  : %.6f\n", lat);
  Serial.printf("  LONGITUDE : %.6f\n", lon);
  Serial.printf("  ALT(m)    : %.1f\n", alt);
  Serial.printf("  SAT       : %d\n",   sat);
  Serial.printf("  BATT(V)   : %.1f\n", batt);
  Serial.printf("  RSSI(dBm) : %d\n",   rssi);
  Serial.printf("  SNR       : %.1f\n", snr);
  Serial.printf("  STATUS    : %s\n",   status_str.c_str());
  Serial.println("----------------------------------------------");

  String jsonPayload = "{";
  jsonPayload += "\"latitude\":"  + String(lat,  6) + ",";
  jsonPayload += "\"longitude\":" + String(lon,  6) + ",";
  jsonPayload += "\"alt\":"       + String(alt,  1) + ",";
  jsonPayload += "\"sat\":"       + String(sat)     + ",";
  jsonPayload += "\"batt\":"      + String(batt, 1) + ",";
  jsonPayload += "\"rssi\":"      + String(rssi)    + ",";
  jsonPayload += "\"snr\":"       + String(snr,  1) + ",";
  jsonPayload += "\"status\":\""  + status_str + "\"";
  jsonPayload += "}";

  Serial.println("[INFO] JSON: " + jsonPayload);

  digitalWrite(LED_K, HIGH);
  bool success = sendHTTPS(jsonPayload);
  digitalWrite(LED_K, LOW);

  if (success) {
    Serial.println("[OK] Data berhasil dikirim ke server!");
    blinkLED(2);
  } else {
    Serial.println("[FAIL] Gagal mengirim data.");
    blinkLED(5);
  }

  Serial.println("==============================================");
  Serial.println("[INFO] Tunggu 30 detik...");
  delay(30000);
}

// ============================================================
// FUNGSI: Kirim Data via HTTPS
// ============================================================
bool sendHTTPS(String payload) {
  Serial.println("[SIM] Membuka koneksi GPRS...");

  sendAT("AT+HTTPTERM", "", 1000);
  delay(500);

  if (!sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 3000)) return false;

  String apnCmd = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  if (!sendAT(apnCmd, "OK", 3000)) return false;

  if (strlen(APN_USER) > 0) {
    sendAT("AT+SAPBR=3,1,\"USER\",\"" + String(APN_USER) + "\"", "OK", 2000);
    sendAT("AT+SAPBR=3,1,\"PWD\",\""  + String(APN_PASS) + "\"", "OK", 2000);
  }

  if (!sendAT("AT+SAPBR=1,1", "OK", 10000)) {
    Serial.println("[WARN] Bearer sudah terbuka, lanjut...");
  }

  sendAT("AT+SAPBR=2,1", "OK", 3000);

  if (!sendAT("AT+HTTPINIT", "OK", 3000)) return false;

  if (!sendAT("AT+HTTPSSL=1", "OK", 2000)) {
    Serial.println("[WARN] HTTPSSL tidak didukung firmware ini!");
  }

  if (!sendAT("AT+HTTPPARA=\"CID\",1", "OK", 2000)) return false;

  String url = "https://" + String(SERVER_HOST) + String(SERVER_PATH);
  if (!sendAT("AT+HTTPPARA=\"URL\",\"" + url + "\"", "OK", 3000)) return false;

  if (!sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", 2000)) return false;

  String dataCmd = "AT+HTTPDATA=" + String(payload.length()) + ",10000";
  if (!sendAT(dataCmd, "DOWNLOAD", 5000)) return false;

  simSerial.print(payload);
  delay(2000);

  String resp = readResponse(3000);
  Serial.println("[SIM RAW] " + resp);

  if (!sendAT("AT+HTTPACTION=1", "OK", 5000)) return false;

  Serial.println("[SIM] Menunggu respons server...");
  resp = readResponse(15000);
  Serial.println("[SIM RESP] " + resp);

  bool success = false;
  if (resp.indexOf("+HTTPACTION: 1,200") >= 0 ||
      resp.indexOf("+HTTPACTION:1,200")  >= 0 ||
      resp.indexOf("+HTTPACTION: 1,201") >= 0 ||
      resp.indexOf("+HTTPACTION:1,201")  >= 0) {
    success = true;
    sendAT("AT+HTTPREAD", "OK", 3000);
  } else {
    Serial.println("[WARN] Respons HTTP tidak 200/201");
  }

  sendAT("AT+HTTPTERM", "OK", 2000);
  return success;
}

// ============================================================
// FUNGSI: Tunggu Registrasi Jaringan
// ============================================================
void waitForNetwork() {
  Serial.println("[SIM] Menunggu registrasi jaringan...");
  for (int i = 0; i < 20; i++) {
    simSerial.println("AT+CREG?");
    String resp = readResponse(3000);
    Serial.println("[SIM] " + resp);
    if (resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0) {
      Serial.println("[OK] Terdaftar di jaringan!");
      return;
    }
    Serial.printf("[INFO] Belum terdaftar, percobaan ke-%d...\n", i + 1);
    delay(3000);
  }
  Serial.println("[WARN] Timeout registrasi jaringan!");
}

// ============================================================
// FUNGSI: Kirim AT Command
// ============================================================
bool sendAT(String cmd, String expected, unsigned long timeout) {
  Serial.println("[AT] >> " + cmd);
  simSerial.println(cmd);
  String resp = readResponse(timeout);
  Serial.println("[AT] << " + resp);

  if (expected == "" || resp.indexOf(expected) >= 0) return true;
  if (resp.indexOf("ERROR") >= 0) {
    Serial.println("[AT] ERROR dari modem!");
    return false;
  }
  return false;
}

// ============================================================
// FUNGSI: Baca Respons SIM900A
// ============================================================
String readResponse(unsigned long timeout) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (simSerial.available()) {
      response += (char)simSerial.read();
    }
    if (response.indexOf("OK")          >= 0 ||
        response.indexOf("ERROR")       >= 0 ||
        response.indexOf("DOWNLOAD")    >= 0 ||
        response.indexOf("+HTTPACTION") >= 0) break;
    delay(10);
  }
  response.trim();
  return response;
}

// ============================================================
// FUNGSI: Kedip LED Indikator
// ============================================================
void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_K, HIGH);
    delay(150);
    digitalWrite(LED_K, LOW);
    delay(150);
  }
}
