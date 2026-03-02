/*
 * ============================================================
 *  SIM900A + Heltec Wireless Tracker v1.2
 *  Kirim Data GPS (acak) via HTTPS POST
 * ============================================================
 *  PIN KONEKSI:
 *    SIM900A TX    --> GPIO 13 (RX ESP32)
 *    SIM900A RX    --> GPIO 14 (TX ESP32)
 *    SIM900A GND   --> GND Heltec
 *    SIM900A VCC   --> VBAT Heltec (3.7V)
 * ============================================================
 */

#include <HardwareSerial.h>

// ---- PIN HELTEC WIRELESS TRACKER v1.2 ----
#define Vext_Ctrl     3
#define LED_K         21
#define LED_BUILTIN   18  // JANGAN dipakai UART

// ---- PIN SIM900A ----
#define SIM_TX        17   // ESP32 TX --> SIM900A RX
#define SIM_RX        18   // ESP32 RX --> SIM900A TX
#define SIM_BAUD      9600
#define SERIAL_BAUD   115200

// ---- KONFIGURASI APN Indosat ----
const char* APN      = "indosatgprs";
const char* APN_USER = "indosat";
const char* APN_PASS = "indosat";

// ---- KONFIGURASI SERVER ----
const char* SERVER_HOST = "laser-tag-project.vercel.app";
const char* SERVER_PATH = "/api/tracker";

HardwareSerial simSerial(1);

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, LOW);   // Aktifkan peripheral onboard
  delay(500);

  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, LOW);

  Serial.println("==============================================");
  Serial.println("  SIM900A HTTPS Tracker - Heltec WT v1.2");
  Serial.println("==============================================");

  simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(2000);

  // --- Cek komunikasi modem ---
  Serial.println("[INIT] Cek komunikasi modem...");
  if (!sendAT("AT", "OK", 3000)) {
    Serial.println("[ERROR] SIM900A tidak merespons! Cek wiring & power.");
    while (true) { blinkLED(5); delay(1000); }
  }
  Serial.println("[OK] Modem merespons!");

  sendAT("ATE0", "OK", 2000);       // Matikan echo
  sendAT("ATI",  "OK", 2000);       // Info firmware

  // --- Cek SIM ---
  Serial.println("[INIT] Cek SIM card...");
  if (!sendAT("AT+CPIN?", "READY", 5000)) {
    Serial.println("[ERROR] SIM card tidak terbaca!");
    while (true) { blinkLED(3); delay(1000); }
  }
  Serial.println("[OK] SIM card OK!");

  // --- Paksa band 900+1800 MHz (Indosat) ---
  Serial.println("[INIT] Set band EGSM+DCS (900+1800 MHz) untuk Indosat...");
  if (sendAT("AT+CBAND=\"EGSM_DCS_MODE\"", "OK", 5000)) {
    Serial.println("[OK] Band berhasil diset ke 900+1800 MHz!");
  } else {
    Serial.println("[WARN] Gagal set band, mencoba ALL_BAND...");
    sendAT("AT+CBAND=\"ALL_BAND\"", "OK", 5000);
  }
  delay(2000);  // Beri waktu modem scan ulang

  // --- Cek sinyal ---
  Serial.println("[INIT] Cek kualitas sinyal...");
  sendAT("AT+CSQ", "OK", 3000);

  // --- Tunggu registrasi jaringan ---
  waitForNetwork();

  // --- Cek operator ---
  sendAT("AT+COPS?", "OK", 5000);

  Serial.println("[INFO] Inisialisasi selesai!");
  Serial.println("==============================================");
  blinkLED(3);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Generate data acak
  float  lat        = -6.2000 + (random(-1000, 1000) / 10000.0);
  float  lon        = 106.8000 + (random(-1000, 1000) / 10000.0);
  float  alt        = 10.0 + random(0, 100);
  int    sat        = random(4, 15);
  float  batt       = 3.5 + (random(0, 12) / 10.0);
  int    rssi       = -(random(50, 110));
  float  snr        = random(-5, 15) + (random(0, 9) / 10.0);
  String status_str = (batt > 3.7) ? "OK" : "LOW_BATT";

  // Tampilkan di Serial Monitor
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

  // Buat JSON
  String json = "{";
  json += "\"latitude\":"  + String(lat,  6) + ",";
  json += "\"longitude\":" + String(lon,  6) + ",";
  json += "\"alt\":"       + String(alt,  1) + ",";
  json += "\"sat\":"       + String(sat)     + ",";
  json += "\"batt\":"      + String(batt, 1) + ",";
  json += "\"rssi\":"      + String(rssi)    + ",";
  json += "\"snr\":"       + String(snr,  1) + ",";
  json += "\"status\":\""  + status_str + "\"";
  json += "}";
  Serial.println("[INFO] JSON: " + json);

  // Cek sinyal sebelum kirim
  Serial.println("[INFO] Cek sinyal sebelum kirim...");
  sendAT("AT+CSQ",   "OK", 2000);
  sendAT("AT+CREG?", "OK", 2000);

  // Kirim data
  digitalWrite(LED_K, HIGH);
  bool ok = sendHTTPS(json);
  digitalWrite(LED_K, LOW);

  if (ok) {
    Serial.println("[OK] Data berhasil dikirim!");
    blinkLED(2);
  } else {
    Serial.println("[FAIL] Gagal kirim, akan coba lagi di loop berikut.");
    blinkLED(5);

    // Jika gagal, coba registrasi ulang
    Serial.println("[RETRY] Cek jaringan ulang...");
    sendAT("AT+CREG?", "OK", 3000);
  }

  Serial.println("==============================================");
  Serial.println("[INFO] Tunggu 30 detik...");
  delay(30000);
}

// ============================================================
// FUNGSI: Kirim via HTTPS
// ============================================================
bool sendHTTPS(String payload) {
  // Tutup sesi sebelumnya
  sendAT("AT+HTTPTERM", "", 1000);
  delay(300);

  // Setup bearer GPRS
  sendAT("AT+SAPBR=0,1", "", 2000);   // Tutup bearer dulu
  delay(500);
  if (!sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 3000)) return false;
  if (!sendAT("AT+SAPBR=3,1,\"APN\",\""  + String(APN)      + "\"", "OK", 3000)) return false;
  if (!sendAT("AT+SAPBR=3,1,\"USER\",\"" + String(APN_USER) + "\"", "OK", 2000)) return false;
  if (!sendAT("AT+SAPBR=3,1,\"PWD\",\""  + String(APN_PASS) + "\"", "OK", 2000)) return false;

  Serial.println("[SIM] Membuka bearer GPRS...");
  if (!sendAT("AT+SAPBR=1,1", "OK", 15000)) {
    Serial.println("[WARN] Gagal buka bearer!");
    return false;
  }

  // Cek IP yang didapat
  sendAT("AT+SAPBR=2,1", "OK", 3000);

  // Init HTTP
  if (!sendAT("AT+HTTPINIT", "OK", 3000)) return false;

  // Aktifkan SSL
  if (!sendAT("AT+HTTPSSL=1", "OK", 2000)) {
    Serial.println("[WARN] SSL tidak didukung, lanjut tanpa SSL...");
  }

  if (!sendAT("AT+HTTPPARA=\"CID\",1", "OK", 2000)) return false;

  String url = "https://" + String(SERVER_HOST) + String(SERVER_PATH);
  Serial.println("[SIM] URL: " + url);
  if (!sendAT("AT+HTTPPARA=\"URL\",\"" + url + "\"", "OK", 3000)) return false;

  if (!sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", 2000)) return false;

  // Input data
  String dataCmd = "AT+HTTPDATA=" + String(payload.length()) + ",10000";
  if (!sendAT(dataCmd, "DOWNLOAD", 5000)) return false;

  simSerial.print(payload);
  delay(2000);
  String r = readResponse(3000);
  Serial.println("[SIM] After HTTPDATA: " + r);

  // POST
  if (!sendAT("AT+HTTPACTION=1", "OK", 5000)) return false;

  Serial.println("[SIM] Menunggu respons server (max 20 detik)...");
  r = readResponse(20000);
  Serial.println("[SIM RESP] " + r);

  bool success = false;
  if (r.indexOf("+HTTPACTION: 1,200") >= 0 || r.indexOf("+HTTPACTION:1,200") >= 0) {
    success = true;
    Serial.println("[OK] HTTP 200 OK");
    sendAT("AT+HTTPREAD", "OK", 3000);
  } else if (r.indexOf("+HTTPACTION: 1,201") >= 0 || r.indexOf("+HTTPACTION:1,201") >= 0) {
    success = true;
    Serial.println("[OK] HTTP 201 Created");
  } else {
    // Parse kode error
    int idx = r.indexOf("+HTTPACTION");
    if (idx >= 0) {
      Serial.println("[ERR] Kode HTTP: " + r.substring(idx));
    }
  }

  sendAT("AT+HTTPTERM", "OK", 2000);
  sendAT("AT+SAPBR=0,1", "", 2000);  // Tutup bearer
  return success;
}

// ============================================================
// FUNGSI: Tunggu Registrasi Jaringan
// ============================================================
void waitForNetwork() {
  Serial.println("[NET] Menunggu registrasi jaringan 2G...");
  for (int i = 0; i < 30; i++) {
    simSerial.println("AT+CREG?");
    String resp = readResponse(3000);
    resp.trim();
    Serial.printf("[NET] Percobaan %2d: %s\n", i + 1, resp.c_str());

    if (resp.indexOf(",1") >= 0) {
      Serial.println("[OK] Terdaftar di jaringan home!");
      sendAT("AT+CSQ",   "OK", 2000);
      sendAT("AT+COPS?", "OK", 3000);
      return;
    }
    if (resp.indexOf(",5") >= 0) {
      Serial.println("[OK] Terdaftar (roaming)!");
      sendAT("AT+CSQ",   "OK", 2000);
      sendAT("AT+COPS?", "OK", 3000);
      return;
    }
    if (resp.indexOf(",3") >= 0) {
      Serial.println("[ERR] Registrasi DITOLAK jaringan!");
      Serial.println("      -> Cek SIM card aktif & tidak expired");
      break;
    }

    // Setiap 10x gagal, coba ganti band
    if (i == 10) {
      Serial.println("[NET] Mencoba ALL_BAND...");
      sendAT("AT+CBAND=\"ALL_BAND\"", "OK", 5000);
    }

    delay(3000);
  }
  Serial.println("[WARN] Timeout registrasi! Lanjut tanpa jaringan...");
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
  if (resp.indexOf("ERROR") >= 0) return false;
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
// FUNGSI: Kedip LED
// ============================================================
void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_K, HIGH);
    delay(150);
    digitalWrite(LED_K, LOW);
    delay(150);
  }
}
