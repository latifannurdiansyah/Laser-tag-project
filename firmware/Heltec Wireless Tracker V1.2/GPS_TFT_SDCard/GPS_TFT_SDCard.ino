#include <TinyGPS++.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>

// Pin definitions
#define Vext_Ctrl 3    // Power GPS + TFT
#define LED_K     21   // Backlight TFT
#define GPS_RX    33
#define GPS_TX    34
#define SD_CS     4    // Chip Select SD Card

// Objects
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(38, 40, 42, 41, 39);
GFXcanvas16 framebuffer = GFXcanvas16(160, 80);
File dataFile;

void setup() {
  // Nyalakan power & backlight
  pinMode(Vext_Ctrl, OUTPUT);
  digitalWrite(Vext_Ctrl, HIGH);
  pinMode(LED_K, OUTPUT);
  digitalWrite(LED_K, HIGH);
  delay(100);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // Inisialisasi TFT
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1);
  framebuffer.fillScreen(ST7735_BLACK);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);

  // Inisialisasi SD Card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Gagal!");
    displayMessage("SD ERROR");
    while(1); // Berhenti selamanya
  }
  
  // Buat file baru (tambah nomor jika sudah ada)
  int fileNum = 1;
  String filename;
  do {
    filename = "/gps_" + String(fileNum) + ".csv";
    fileNum++;
  } while(SD.exists(filename.c_str()));
  
  dataFile = SD.open(filename.c_str(), FILE_WRITE);
  
  if (dataFile) {
    dataFile.println("lat,lon,sats,alt,time");
    dataFile.close();
    Serial.println("File GPS dibuat: " + filename);
    displayMessage("SD READY");
    delay(1000);
  }
}

void loop() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 5000) { // Simpan tiap 5 detik
    lastLog = millis();

    // Update TFT
    updateDisplay();

    // Simpan ke SD Card
    if (gps.location.isValid()) {
      dataFile = SD.open("/gps_1.csv", FILE_APPEND); // Ambil file pertama
      if (dataFile) {
        dataFile.print(gps.location.lat(), 6);
        dataFile.print(",");
        dataFile.print(gps.location.lng(), 6);
        dataFile.print(",");
        dataFile.print(gps.satellites.value());
        dataFile.print(",");
        dataFile.print(gps.altitude.meters(), 1);
        dataFile.print(",");
        dataFile.println(millis()); // Waktu dalam ms sejak nyala
        dataFile.close();
        
        Serial.println("Data tersimpan ke SD");
      }
    }
  }
}

void updateDisplay() {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextColor(ST7735_WHITE);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(5, 5);
  framebuffer.print("GPS LOGGER");

  if (gps.location.isValid()) {
    framebuffer.setCursor(5, 20);
    framebuffer.print("Lat:");
    framebuffer.print(gps.location.lat(), 5);

    framebuffer.setCursor(5, 35);
    framebuffer.print("Lon:");
    framebuffer.print(gps.location.lng(), 5);

    framebuffer.setCursor(5, 50);
    framebuffer.print("Sats:");
    framebuffer.print(gps.satellites.value());
    framebuffer.print(" Alt:");
    framebuffer.print((int)gps.altitude.meters());
    framebuffer.print("m");
  } else {
    framebuffer.setCursor(25, 25);
    framebuffer.setTextColor(ST7735_RED);
    framebuffer.setTextSize(2);
    framebuffer.print("NO FIX");
    framebuffer.setTextSize(1);
  }

  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}

void displayMessage(String msg) {
  framebuffer.fillScreen(ST7735_BLACK);
  framebuffer.setTextColor(ST7735_YELLOW);
  framebuffer.setTextSize(2);
  framebuffer.setCursor(10, 30);
  framebuffer.print(msg);
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), 160, 80);
}