#include <HardwareSerial.h>

// Pin Definitions (sesuai config.h)
#define Vext_Ctrl 3    // Kontrol daya eksternal - WAJIB diaktifkan!
#define LED_K 21       // LED indikator board
#define GPRS_TX_PIN 17 // ESP32 TX → SIM900A RX
#define GPRS_RX_PIN 16 // ESP32 RX ← SIM900A TX
#define GPRS_RST_PIN 15

HardwareSerial sim900(1); // UART1 untuk SIM900A

const char* APN = "indosatgprs";
const char* SERVER = "new-simpera.isar.web.id";
const uint16_t SERVER_PORT = 80;

void setup() {
  // === WAJIB: Aktifkan daya eksternal ===
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, LOW);  // LOW = ON (aktifkan Vext untuk SIM900A)
  delay(100);

  // LED indikator
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH); // Matikan LED awal

  // Serial monitoring minimal (USB)
  Serial.begin(115200);
  Serial.println("SIM900A - Heltec Tracker");

  // Inisialisasi serial ke SIM900A
  sim900.begin(9600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);

  // Reset hardware SIM900A
  pinMode(GPRS_RST_PIN, OUTPUT);
  digitalWrite(GPRS_RST_PIN, LOW);
  delay(200);
  digitalWrite(GPRS_RST_PIN, HIGH);
  delay(3000);

  // Inisialisasi modem (tanpa Serial.println berlebihan)
  kirimAT("AT");
  delay(1000);
  kirimAT("AT+CPIN?");
  delay(1000);
  kirimAT("AT+CREG?");
  delay(1500);
  
  // Perbaikan: gunakan String untuk gabungkan APN dengan aman
  String cmdAPN = "AT+CSTT=\"";
  cmdAPN += APN;
  cmdAPN += "\"";
  kirimAT(cmdAPN.c_str());
  
  delay(1000);
  kirimAT("AT+CIICR");
  delay(3000);
  kirimAT("AT+CIFSR");
  delay(1000);
}

void loop() {
  digitalWrite(LED_K, LOW);   // LED ON - proses kirim
  kirimData("ID707", -6.2146, 106.8452);
  digitalWrite(LED_K, HIGH);  // LED OFF - idle
  
  delay(30000); // Interval 30 detik
}

// Kirim perintah AT tanpa echo berlebihan
void kirimAT(const char* cmd) {
  sim900.println(cmd);
  delay(500);
  while (sim900.available()) sim900.read(); // Kosongkan buffer
}

// Kirim data ke server via HTTP POST
void kirimData(const char* id, float lat, float lon) {
  // Format data
  String body = "address_id=";
  body += id;
  body += "&lat=";
  body += String(lat, 6);
  body += "&lon=";
  body += String(lon, 6);

  // Buka koneksi TCP
  sim900.print("AT+CIPSTART=\"TCP\",\"");
  sim900.print(SERVER);
  sim900.print("\",");
  sim900.println(SERVER_PORT);
  delay(4000);

  // Kirim data
  sim900.print("AT+CIPSEND=");
  sim900.println(body.length() + 65); // +65 untuk header HTTP
  delay(1000);

  if (sim900.find(">")) {
    sim900.print("POST /end_point_heltec-tracker_table.php HTTP/1.1\r\n");
    sim900.print("Host: ");
    sim900.print(SERVER);
    sim900.print("\r\nContent-Length: ");
    sim900.print(body.length());
    sim900.print("\r\n\r\n");
    sim900.print(body);
    delay(2000);
    sim900.println("AT+CIPCLOSE");
  } else {
    sim900.println("AT+CIPCLOSE");
  }
}