# Heltec Tracker – GPS + LoRa + ESP-NOW Firmware

Firmware untuk Heltec ESP32-S3 dengan integrasi:
- **GPS** (UBlox NEO)
- **LoRa P2P** (SX1262 @ 923 MHz)
- **ESP-NOW** (penerima data IR dari sensor jarak jauh)

Mengirim data lokasi GPS dan kode inframerah (IR) secara real-time melalui LoRa.

---

## Fitur Utama

- Terima data IR via **ESP-NOW** dari node lain (misal: sensor tembakan Laser Tag)
- Baca data **GPS real-time**
- Pantau **tegangan baterai**
- Kirim gabungan data **GPS + IR** via **LoRa**
- Tampilan status real-time di **layar TFT ST7735 (160x80)**
- LED indikator TX LoRa
- Random backoff untuk hindari collision
- Reset otomatis data IR setelah 5 detik

---

## Struktur File
heltec-tracker-espnow-lora/
├── README.md
├── PinConfig.h
├── heltec-tracker-espnow-lora.ino
├── ESPNowModule.ino
├── LoRaModule.ino
├── GPSModule.ino
├── TFTDisplay.ino
└── (tidak ada Utils.ino — fungsi sederhana langsung di modul)


---

## Konfigurasi Penting

### Di `PinConfig.h`:

- Ubah `senderMac[]` sesuai MAC address pengirim ESP-NOW
- Sesuaikan `NODE_ID` jika ada multi-node
- Atur frekuensi LoRa sesuai wilayah (923.0 untuk Indonesia)

### Payload LoRa (22 byte)

| Field | Ukuran | Deskripsi |
|-------|--------|----------|
| Header | 1B | `0xAA` |
| Group ID | 1B | Filter grup |
| Node ID | 1B | ID pengirim |
| Latitude | 4B | `int32`, ×1e6 |
| Longitude | 4B | `int32`, ×1e6 |
| Altitude | 2B | meter |
| Satellites | 1B | jumlah satelit |
| Battery | 2B | mV |
| Uptime | 4B | detik sejak boot |
| IR Address | 2B | alamat NEC |
| IR Command | 1B | perintah NEC |
| Hit Status | 1B | 1 = hit, 0 = idle |
| Checksum | 1B | XOR seluruh byte kecuali checksum |

Total: **22 byte**

---

## Dependensi Library

- RadioLib
- TinyGPSPlus
- Adafruit GFX Library
- Adafruit ST7735 Library

Install via Arduino IDE → **Tools → Manage Libraries**

---

## Cara Penggunaan

1. Upload ke **Heltec ESP32 (S3)** sebagai **receiver ESP-NOW + sender LoRa**
2. Siapkan node lain (misal ESP32 lain) yang mengirim data IR via ESP-NOW
3. Nyalakan GPS dan pastikan fix
4. Data akan dikirim tiap ~3 detik + backoff acak
5. Jika ada IR baru, kirim segera

---

## Indikator TFT

- **IR**: Menampilkan `0xADDR/CMD` atau "Waiting..."
- **GPS**: Warna kuning (≥4 sat), merah (<4 sat)
- **TX Counter**: Jumlah transmisi & error

---

## Catatan

- ESP-NOW hanya bekerja dalam mode **STA** (tidak bisa AP)
- Pastikan channel WiFi sama antara pengirim & penerima ESP-NOW
- Gunakan `VEXT_CTRL = HIGH` untuk aktifkan regulator eksternal