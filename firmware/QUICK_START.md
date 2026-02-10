# 🚀 PANDUAN CEPAT - HELTEC TRACKER ALL-IN-ONE

## ✅ File Tunggal - Tidak Perlu File Terpisah!

File `heltec_tracker_all_in_one.ino` sudah berisi **SEMUA** kode dan konfigurasi dalam satu file.

---

## 📝 LANGKAH INSTALASI

### 1. **Edit Konfigurasi WiFi** (Baris 19-23)
```cpp
#define WIFI_SSID       "UserAndroid"        // ← Ganti dengan WiFi Anda
#define WIFI_PASSWORD   "55555550"          // ← Ganti password WiFi
#define API_URL         "https://laser-tag-project.vercel.app/api/track"
#define DEVICE_ID       "Player 1"
#define IR_RECEIVE_PIN  5
```

### 2. **Edit Kredensial LoRaWAN dari TTN** (Baris 32-50)

Buka TTN Console → Applications → Devices → [Your Device]

**PENTING:** 
- DevEUI dan JoinEUI dalam format **LSB** (Least Significant Byte)
- AppKey dan NwkKey dalam format **MSB** (Most Significant Byte)

```cpp
const uint64_t joinEUI = 0x0000000000000011;  // ← Dari TTN (LSB)
const uint64_t devEUI  = 0x70B3D57ED00739E0;  // ← Dari TTN (LSB)

const uint8_t appKey[] = {
    0xDB, 0x51, 0xEB, 0x2C, 0x50, 0x13, 0x84, 0x82,  // ← Dari TTN (MSB)
    0xE2, 0xFB, 0xD2, 0x64, 0xE7, 0x87, 0x64, 0x27
};

const uint8_t nwkKey[] = {
    0x84, 0x0E, 0x4C, 0x4C, 0xD0, 0x87, 0xC8, 0x39,  // ← Dari TTN (MSB)
    0x56, 0x58, 0x04, 0xE0, 0x39, 0xA7, 0x36, 0x16
};
```

### 3. **Sesuaikan Region** (Baris 94-95)
```cpp
const LoRaWANBand_t Region = AS923;  // ← AS923 untuk Indonesia
const uint8_t subBand = 0;
```

**Pilihan Region:**
- `EU868` - Eropa
- `US915` - Amerika Utara
- `AU915` - Australia
- `AS923` - Asia Tenggara (Indonesia)

### 4. **Sesuaikan Timezone** (Baris 70)
```cpp
const int8_t TIMEZONE_OFFSET_HOURS = 7;  // ← WIB=+7, WITA=+8, WIT=+9
```

---

## 📚 LIBRARY YANG DIPERLUKAN

Install via Arduino Library Manager:

1. **RadioLib** (by Jan Gromeš)
2. **Adafruit ST7735 and ST7789**
3. **Adafruit GFX Library**
4. **TinyGPSPlus** (atau gunakan `HT_TinyGPS.h` dari Heltec)
5. **IRremote** (by shirriff)
6. Library built-in ESP32:
   - WiFi
   - HTTPClient
   - SD
   - SPI

---

## 🔧 CARA UPLOAD

1. Buka file `heltec_tracker_all_in_one.ino` di Arduino IDE
2. Edit konfigurasi di bagian atas (WiFi, LoRaWAN, Region, Timezone)
3. Pilih Board: **Heltec Wireless Tracker** atau **ESP32S3 Dev Module**
4. Pilih Port COM yang sesuai
5. Klik **Upload** ✅

---

## 📊 FITUR PROGRAM

### ✅ Multi-Task FreeRTOS:
- **GPS Task** - Membaca data GPS setiap 1 detik
- **LoRaWAN Task** - Kirim data ke TTN setiap 30 detik
- **WiFi Task** - Kirim data ke API Vercel setiap 10 detik
- **IR Task** - Deteksi sinyal IR NEC protocol
- **TFT Task** - Update display setiap 100ms, ganti halaman setiap 3 detik
- **SD Card Task** - Logging data setiap 2 detik

### 📱 TFT Display (3 Halaman):
- **Page 1:** LoRaWAN Status (Event, RSSI, SNR, Battery)
- **Page 2:** GPS Status (Lat, Long, Alt, Satellites, Time)
- **Page 3:** IR Receiver (Protocol, Address, Command)

### 📡 LoRaWAN Payload:
```cpp
struct DataPayload {
    uint32_t address_id;            // Device ID (0x12345678)
    uint8_t sub_address_id;         // IR Command
    uint32_t shooter_address_id;    // IR Address
    uint8_t shooter_sub_address_id; // IR flag
    uint8_t status;                 // 0=normal, 1=hit
    float lat, lng, alt;            // GPS coordinates
};
```
**Total size:** 25 bytes

---

## 🐛 TROUBLESHOOTING

### ❌ LoRaWAN Join Gagal

**Cek Serial Monitor:**
```
Join failed: ERR_NO_JOIN_ACCEPT
```

**Solusi:**
1. ✅ Pastikan DevEUI, JoinEUI, AppKey, NwkKey **BENAR**
2. ✅ Cek format LSB/MSB sudah sesuai
3. ✅ Device di TTN dalam status "Activated" (bukan Suspended)
4. ✅ Frequency Plan di TTN = AS923
5. ✅ Ada gateway TTN dalam jangkauan (cek TTN Map)
6. ✅ Antenna LoRa terpasang dengan baik

**Tip Konversi DevEUI:**
TTN Console menampilkan: `70B3D57ED00739E0` (MSB)  
RadioLib butuh: `0x70B3D57ED00739E0` (sudah LSB, tinggal copy!)

### ❌ GPS Tidak Dapat Fix

**Solusi:**
- Pastikan outdoor atau dekat jendela
- Tunggu 2-5 menit untuk cold start
- Cek antenna GPS terpasang

### ❌ WiFi Tidak Connect

**Solusi:**
- Pastikan WiFi **2.4GHz** (bukan 5GHz)
- Cek SSID dan password benar
- Cek sinyal WiFi cukup kuat

### ❌ SD Card Error

**Solusi:**
- Format SD card ke **FAT32**
- Maksimal 32GB
- Pastikan SD card terpasang dengan benar

---

## 📈 EXPECTED OUTPUT

### Serial Monitor:
```
========================================
GPS Tracker + IR + LoRaWAN + WiFi + SD
Device ID: Player 1
API URL: https://laser-tag-project.vercel.app/api/track
========================================

GNSS module powered and Serial1 started.
IR Receiver Ready on Pin 5
Starting OTAA join...
OTAA attempt 1
✅ OTAA Join successful!
Connecting to WiFi: UserAndroid
WiFi Connected!
IP Address: 192.168.1.100
SD Card initialized successfully
```

### TTN Console (Live Data):
```
Received uplink
Payload: 78563412 FF 3412CDAB 01 01 [GPS data]
```

### TFT Display:
- Status bar menampilkan: `W:OK L:OK` (WiFi dan LoRa connected)
- Rotasi otomatis setiap 3 detik: LoRa → GPS → IR

---

## 📞 SUPPORT

Jika ada error, cek:
1. **Serial Monitor** - Output debug lengkap
2. **TTN Console** → Device → Live Data
3. **SD Card** - File `/tracker.log`

---

## ⚡ QUICK CHECKLIST

- [ ] Library sudah terinstall semua
- [ ] WiFi SSID & Password sudah diganti
- [ ] LoRaWAN credentials dari TTN sudah benar
- [ ] Region = AS923 untuk Indonesia
- [ ] Timezone = +7 (WIB)
- [ ] Board = Heltec Wireless Tracker
- [ ] Antenna GPS & LoRa terpasang
- [ ] SD Card terformat FAT32
- [ ] Upload berhasil

Good luck! 🚀
