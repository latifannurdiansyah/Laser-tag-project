#include <Arduino.h>

// ================= KONFIG =================
#define APN       "indosatgprs"
#define SERVER    "http://IP_VPS_KAMU/gsm"
#define DEVICE_ID "tracker_01"

// UART2 SIM900A
HardwareSerial sim900(2);

// ================= AT HELPER =================
void sendAT(const String &cmd, uint32_t waitMs = 1500) {
  sim900.println(cmd);
  unsigned long t = millis();
  while (millis() - t < waitMs) {
    while (sim900.available()) {
      Serial.write(sim900.read());
    }
  }
  Serial.println();
}

// ================= RSSI =================
int getRSSI() {
  sim900.flush();
  sim900.println("AT+CSQ");
  delay(1200);

  String resp = "";
  while (sim900.available()) {
    resp += (char)sim900.read();
  }

  Serial.println("CSQ RAW: " + resp);

  int idx = resp.indexOf("+CSQ:");
  if (idx < 0) return -99;

  int comma = resp.indexOf(",", idx);
  int csq = resp.substring(idx + 6, comma).toInt();

  if (csq == 99) return -99;
  return -113 + (csq * 2);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(2000);

  sim900.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("\n=== SIM900A HTTP TEST START ===");

  sendAT("AT");
  sendAT("AT+CPIN?");
  sendAT("AT+CSQ");

  // Attach GPRS
  sendAT("AT+CGATT=1", 3000);

  // PDP Context
  sendAT("AT+CSTT=\"" APN "\",\"\",\"\"");
  sendAT("AT+CIICR", 8000);

  // CEK IP (WAJIB ADA)
  Serial.println("=== CHECK IP ===");
  sim900.println("AT+CIFSR");
  delay(3000);
  while (sim900.available()) {
    Serial.write(sim900.read());
  }
  Serial.println("\n=== END IP CHECK ===");

  // Reset HTTP engine
  sendAT("AT+HTTPTERM", 2000);
  delay(1000);
  sendAT("AT+HTTPINIT", 2000);
  sendAT("AT+HTTPPARA=\"CID\",1");
  sendAT("AT+HTTPPARA=\"URL\",\"" SERVER "\"");
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
}

// ================= LOOP =================
void loop() {
  int rssi = getRSSI();

  if (rssi < -95) {
    Serial.println("SIGNAL TOO WEAK, SKIP HTTP");
    delay(15000);
    return;
  }

  // ===== DATA TEST =====
  float lat = -6.200100;
  float lon = 106.816597;
  float alt = 12.5;
  int   sat = 7;
  float snr = 7.0;
  String status = "alive";

  // ===== JSON =====
  String json = "{";
  json += "\"device_id\":\"" DEVICE_ID "\",";
  json += "\"lat\":" + String(lat, 6) + ",";
  json += "\"lon\":" + String(lon, 6) + ",";
  json += "\"alt\":" + String(alt, 1) + ",";
  json += "\"sat\":" + String(sat) + ",";
  json += "\"rssi\":" + String(rssi) + ",";
  json += "\"snr\":" + String(snr, 1) + ",";
  json += "\"status\":\"" + status + "\"";
  json += "}";

  Serial.println("\n=== SENDING JSON ===");
  Serial.println(json);

  // ===== SEND BODY =====
  sim900.print("AT+HTTPDATA=");
  sim900.print(json.length());
  sim900.println(",10000");
  delay(2000);

  sim900.print(json);
  delay(2000);

  // ===== HTTP POST =====
  Serial.println("=== WAIT HTTPACTION ===");
  sim900.println("AT+HTTPACTION=1");

  unsigned long t = millis();
  while (millis() - t < 20000) {   // max 20 detik
    if (sim900.available()) {
      Serial.write(sim900.read());
    }
  }

  Serial.println("\n=== END HTTPACTION ===");

  sendAT("AT+HTTPTERM", 2000);

  Serial.println("=== DONE, WAIT 20s ===\n");
  delay(20000);
}