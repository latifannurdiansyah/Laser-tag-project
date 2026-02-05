#include <WiFi.h>
#include <HTTPClient.h>

#define WIFI_SSID "Polinema Hostspot"
#define WIFI_PASSWORD ""

const char* serverName = "http://192.168.11.188:3000/api/track";

void setup() {
  Serial.begin(115200);

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Ready to send random coordinates");
}

void loop() {
  static unsigned long lastSend = 0;
  const unsigned long interval = 5000;

  if (millis() - lastSend >= interval) {
    lastSend = millis();

    float lat = -8.0 + ((float)random(1000) / 1000.0) * 15.0;
    float lng = 95.0 + ((float)random(1000) / 1000.0) * 30.0;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");

      String jsonData = "{\"id\":\"HELTEC-01\",\"lat\":\"" + String(lat, 6) + "\",\"lng\":\"" + String(lng, 6) + "\"}";

      int httpResponseCode = http.POST(jsonData);

      Serial.print("Sent: lat="); Serial.print(lat, 6);
      Serial.print(", lng="); Serial.print(lng, 6);
      Serial.print(" | Response: "); Serial.println(httpResponseCode);

      http.end();
    } else {
      Serial.println("WiFi disconnected - reconnecting...");
      WiFi.reconnect();
    }
  }
}