/*
 * ============================================================
 *  SIM900A + Heltec Wireless Tracker v1.2
 *  Kirim Data GPS (acak) via HTTPS POST
 * ============================================================
 *  PIN KONEKSI:
 *    SIM900A TX  --> ESP32 RX (GPIO 18)
 *    SIM900A RX  --> ESP32 TX (GPIO 17)
 *    SIM900A GND --> ESP32 GND
 *    SIM900A VCC --> 4.0V (WAJIB gunakan power supply 2A)
 * ============================================================
 *  CATATAN: SIM900A tidak mendukung TLS/HTTPS secara native.
 *  Solusi: gunakan HTTP biasa ke server proxy/middleware,
 *  ATAU gunakan AT+HTTPSSL=1 jika firmware mendukung.
 *  Kode ini menggunakan AT+HTTPSSL=1 (beberapa modul SIM900A
 *  sudah mendukung jika firmware >= R14.18)
 * ============================================================
 */

#include <HardwareSerial.h>

// ---- KONFIGURASI ----
#define SIM_TX       17       // ESP32 TX  --> SIM900A RX
#define SIM_RX       18       // ESP32 RX  --> SIM900A TX
#define SIM_BAUD     9600
#define SERIAL_BAUD  115200

// Ganti dengan APN operator Anda
const char* APN    = "indosatgprs";       // Contoh: "internet" (Telkomsel), "internet.tri.id" (3), dsb
const char* APN_USER = "indosat";
const char* APN_PASS = "indosat";

// Ganti dengan endpoint server HTTPS Anda
const char* SERVER_HOST = "laser-tag-project.vercel.app";   // tanpa https://
const int   SERVER_PORT = 443;
const char* SERVER_PATH = "/api/tracker";     // path endpoint

// ---- VARIABEL ----
HardwareSerial simSerial(1);  // UART1

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println("==============================================");
  Serial.println("  SIM900A HTTPS Tracker - Heltec WT v1.2");
  Serial.println("==============================================");

  simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(2000);

  Serial.println("[INFO] Inisialisasi SIM900A...");

  // Cek modem
  sendAT("AT", "OK", 3000);

  // Matikan echo
  sendAT("ATE0", "OK", 2000);

  // Cek SIM
  sendAT("AT+CPIN?", "READY", 5000);

  // Cek registrasi jaringan
  waitForNetwork();

  // Cek kualitas sinyal
  sendAT("AT+CSQ", "OK", 2000);

  Serial.println("[INFO] Inisialisasi selesai. Memulai loop...");
  Serial.println("==============================================");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Generate data acak (simulasi GPS + sensor)
  float lat   = -6.2000 + (random(-1000, 1000) / 10000.0);
  float lon   = 106.8000 + (random(-1000, 1000) / 10000.0);
  float alt   = 10.0 + random(0, 100);
  int   sat   = random(4, 15);
  float batt  = 3.5 + (random(0, 12) / 10.0);
  int   rssi  = -(random(50, 110));
  float snr   = random(-5, 15) + (random(0, 9) / 10.0);
  String status_str = (batt > 3.7) ? "OK" : "LOW_BATT";

  // Tampilkan di Serial Monitor
  Serial.println("----------------------------------------------");
  Serial.println("[DATA] Payload yang akan dikirim:");
  Serial.printf("  LATITUDE  : %.6f\n", lat);
  Serial.printf("  LONGITUDE : %.6f\n", lon);
  Serial.printf("  ALT(m)    : %.1f\n", alt);
  Serial.printf("  SAT       : %d\n", sat);
  Serial.printf("  BATT(V)   : %.1f\n", batt);
  Serial.printf("  RSSI(dBm) : %d\n", rssi);
  Serial.printf("  SNR       : %.1f\n", snr);
  Serial.printf("  STATUS    : %s\n", status_str.c_str());
  Serial.println("----------------------------------------------");

  // Buat JSON payload
  String jsonPayload = "{";
  jsonPayload += "\"latitude\":"  + String(lat, 6) + ",";
  jsonPayload += "\"longitude\":" + String(lon, 6) + ",";
  jsonPayload += "\"alt\":"       + String(alt, 1) + ",";
  jsonPayload += "\"sat\":"       + String(sat)    + ",";
  jsonPayload += "\"batt\":"      + String(batt, 1) + ",";
  jsonPayload += "\"rssi\":"      + String(rssi)   + ",";
  jsonPayload += "\"snr\":"       + String(snr, 1) + ",";
  jsonPayload += "\"status\":\""  + status_str     + "\"";
  jsonPayload += "}";

  Serial.println("[INFO] JSON: " + jsonPayload);

  // Kirim via HTTPS
  bool success = sendHTTPS(jsonPayload);

  if (success) {
    Serial.println("[OK] Data berhasil dikirim ke server!");
  } else {
    Serial.println("[FAIL] Gagal mengirim data. Coba lagi...");
  }

  Serial.println("==============================================");
  Serial.println("[INFO] Tunggu 30 detik...");
  delay(30000);  // kirim setiap 30 detik
}

// ============================================================
// FUNGSI: Kirim Data via HTTPS (menggunakan AT Commands HTTP)
// ============================================================
bool sendHTTPS(String payload) {
  Serial.println("[SIM] Membuka koneksi GPRS...");

  // Tutup bearer jika masih aktif
  sendAT("AT+HTTPTERM", "", 1000);
  delay(500);

  // Inisialisasi bearer (GPRS)
  if (!sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 3000)) return false;

  String apnCmd = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  if (!sendAT(apnCmd, "OK", 3000)) return false;

  if (strlen(APN_USER) > 0) {
    String userCmd = "AT+SAPBR=3,1,\"USER\",\"" + String(APN_USER) + "\"";
    sendAT(userCmd, "OK", 2000);
    String passCmd = "AT+SAPBR=3,1,\"PWD\",\"" + String(APN_PASS) + "\"";
    sendAT(passCmd, "OK", 2000);
  }

  // Buka bearer
  if (!sendAT("AT+SAPBR=1,1", "OK", 10000)) {
    Serial.println("[WARN] Bearer sudah terbuka atau gagal, mencoba lanjut...");
  }

  // Cek IP
  sendAT("AT+SAPBR=2,1", "OK", 3000);

  // Inisialisasi HTTP
  if (!sendAT("AT+HTTPINIT", "OK", 3000)) return false;

  // Set HTTPS SSL
  if (!sendAT("AT+HTTPSSL=1", "OK", 2000)) {
    Serial.println("[WARN] HTTPSSL tidak didukung, mencoba tanpa SSL...");
    // Jika tidak support SSL, ubah port ke 80 dan gunakan http
  }

  // Set parameter HTTP
  if (!sendAT("AT+HTTPPARA=\"CID\",1", "OK", 2000)) return false;

  // Buat URL lengkap
  String url = "https://" + String(SERVER_HOST) + String(SERVER_PATH);
  String urlCmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
  if (!sendAT(urlCmd, "OK", 3000)) return false;

  // Set Content-Type JSON
  if (!sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", 2000)) return false;

  // Set panjang data
  int payloadLen = payload.length();
  String dataCmd = "AT+HTTPDATA=" + String(payloadLen) + ",10000";
  if (!sendAT(dataCmd, "DOWNLOAD", 5000)) return false;

  // Kirim payload
  simSerial.print(payload);
  delay(2000);

  // Tunggu konfirmasi
  String resp = readResponse(3000);
  Serial.println("[SIM RAW] " + resp);

  // POST request
  if (!sendAT("AT+HTTPACTION=1", "OK", 5000)) return false;

  // Tunggu respons HTTP (biasanya butuh 5-15 detik)
  Serial.println("[SIM] Menunggu respons server...");
  resp = readResponse(15000);
  Serial.println("[SIM RESP] " + resp);

  bool success = false;
  if (resp.indexOf("+HTTPACTION: 1,200") >= 0 ||
      resp.indexOf("+HTTPACTION:1,200") >= 0) {
    success = true;
    // Baca respons body
    sendAT("AT+HTTPREAD", "OK", 3000);
  } else if (resp.indexOf("+HTTPACTION: 1,201") >= 0) {
    success = true;
    Serial.println("[INFO] HTTP 201 Created");
  } else {
    Serial.println("[WARN] Respons HTTP tidak 200/201");
  }

  // Terminasi HTTP
  sendAT("AT+HTTPTERM", "OK", 2000);

  return success;
}

// ============================================================
// FUNGSI: Tunggu Registrasi Jaringan
// ============================================================
void waitForNetwork() {
  Serial.println("[SIM] Menunggu registrasi jaringan...");
  for (int i = 0; i < 5; i++) {
    simSerial.println("AT+CREG?");
    String resp = readResponse(3000);
    Serial.println("[SIM] " + resp);
    // 0,1 = registered home, 0,5 = roaming
    if (resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0) {
      Serial.println("[OK] Terdaftar di jaringan!");
      return;
    }
    Serial.printf("[INFO] Belum terdaftar, coba ke-%d...\n", i + 1);
    delay(3000);
  }
  Serial.println("[WARN] Timeout registrasi jaringan!");
}

// ============================================================
// FUNGSI: Kirim AT Command dan tunggu respons
// ============================================================
bool sendAT(String cmd, String expected, unsigned long timeout) {
  Serial.println("[AT] >> " + cmd);
  simSerial.println(cmd);
  String resp = readResponse(timeout);
  Serial.println("[AT] << " + resp);

  if (expected == "" || resp.indexOf(expected) >= 0) {
    return true;
  }
  if (resp.indexOf("ERROR") >= 0) {
    Serial.println("[AT] ERROR dari modem!");
    return false;
  }
  return false;
}

// ============================================================
// FUNGSI: Baca respons dari SIM900A
// ============================================================
String readResponse(unsigned long timeout) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (simSerial.available()) {
      char c = simSerial.read();
      response += c;
    }
    // Jika sudah ada "OK" atau "ERROR", berhenti lebih cepat
    if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0 ||
        response.indexOf("DOWNLOAD") >= 0 || response.indexOf("+HTTPACTION") >= 0) {
      break;
    }
    delay(10);
  }
  response.trim();
  return response;
}
