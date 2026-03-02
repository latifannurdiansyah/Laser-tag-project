# GPS Tracker dengan SD Card Logging - Dokumentasi

## Fitur yang Ditambahkan

### 1. Pin SD Card
```cpp
#define SD_CS 4  // Pin Chip Select untuk SD Card
```

### 2. Library yang Ditambahkan
```cpp
#include <SD.h>  // Library untuk SD Card
```

### 3. Fungsi-Fungsi Baru

#### a. `initSDCard()`
Menginisialisasi SD Card dan menampilkan informasi:
- Tipe kartu (MMC, SD, SDHC)
- Ukuran kartu dalam MB
- Status keberhasilan inisialisasi

#### b. `generateLogFileName()`
Membuat nama file log otomatis berdasarkan tanggal GPS:
- Format: `/GPS_DDMMYY.csv`
- Contoh: `/GPS_090226.csv` (9 Februari 2026)
- Default: `/GPS_LOG.csv` (jika tanggal GPS belum tersedia)

#### c. `writeCSVHeader()`
Menulis header CSV ke file baru dengan kolom:
- Timestamp (milliseconds)
- Date (YYYY-MM-DD)
- Time (HH:MM:SS)
- Latitude
- Longitude
- Altitude (meter)
- Satellites
- Speed (km/h)
- Course (derajat)
- HDOP

#### d. `logGPSToSD()`
Fungsi utama untuk logging data GPS ke SD Card:
- Membuat file baru jika belum ada
- Menambahkan data ke file yang sudah ada
- Mengganti file saat ganti hari
- Menyimpan semua data GPS yang tersedia

### 4. Interval Logging
```cpp
const unsigned long SD_LOG_INTERVAL = 5000; // 5 detik
```
Anda dapat mengubah nilai ini sesuai kebutuhan:
- 1000 = 1 detik (logging lebih sering, file lebih besar)
- 5000 = 5 detik (default, balanced)
- 10000 = 10 detik (hemat ruang SD Card)

### 5. Format Data CSV
Contoh data yang tersimpan:
```
Timestamp,Date,Time,Latitude,Longitude,Altitude,Satellites,Speed,Course,HDOP
1234567,2026-02-09,14:30:15,-7.276508,112.795105,125.50,8,0.00,0.00,1.20
1239567,2026-02-09,14:30:20,-7.276510,112.795107,125.60,9,5.50,45.30,1.10
```

## Penggunaan

### Setup Hardware
1. Pasang SD Card ke slot SD Card di Heltec Tracker
2. Pastikan SD Card terformat FAT32
3. Gunakan SD Card yang bagus (Class 10 atau lebih)

### Monitoring
Program akan menampilkan di layar TFT:
- **SD:OK** (hijau) - SD Card terdeteksi dan berfungsi
- **SD:X** (merah) - SD Card tidak terdeteksi
- **SD Logging...** (cyan) - Proses logging sedang berjalan
- LED Built-in akan berkedip setiap kali data berhasil disimpan

### Serial Monitor
Informasi yang ditampilkan:
```
Initializing SD card...
SD Card Type: SDHC
SD Card Size: 16384MB
CSV header written
GPS logged: 1234567,2026-02-09,14:30:15,-7.276508,112.795105,125.50,8,0.00,0.00,1.20
```

## Troubleshooting

### SD Card tidak terdeteksi
1. Pastikan SD Card terpasang dengan benar
2. Format SD Card ke FAT32
3. Coba SD Card lain
4. Periksa koneksi pin CS (pin 4)

### File tidak terbuat
1. Pastikan SD Card tidak write-protected
2. Periksa apakah SD Card masih memiliki ruang kosong
3. Coba format ulang SD Card

### Data tidak tersimpan
1. Pastikan GPS sudah mendapat fix (valid location)
2. Periksa Serial Monitor untuk pesan error
3. Pastikan interval logging sudah sesuai

## Membaca Data Log

### Dari SD Card
1. Cabut SD Card dari Heltec Tracker
2. Masukkan ke card reader komputer
3. Buka file CSV dengan:
   - Microsoft Excel
   - Google Sheets
   - Text Editor
   - Python/MATLAB untuk analisis data

### Contoh Import ke Excel
1. Open Excel
2. Data → Get Data → From File → From Text/CSV
3. Pilih file GPS_*.csv
4. Data akan otomatis terpisah ke kolom-kolom

## Estimasi Ukuran File

Dengan interval 5 detik:
- 1 jam = 720 entries ≈ 50 KB
- 1 hari = 17,280 entries ≈ 1.2 MB
- 1 minggu = 120,960 entries ≈ 8.4 MB

SD Card 8GB dapat menyimpan data GPS sekitar 1000 hari (3 tahun lebih)!

## Tips Optimasi

### Hemat Ruang SD Card
1. Tingkatkan interval logging (misal 10-30 detik)
2. Hapus data lama secara berkala
3. Kompres file CSV sebelum backup

### Logging Selektif
Tambahkan kondisi untuk logging hanya saat:
- Kecepatan > 0 (saat bergerak)
- HDOP < 2.0 (akurasi bagus)
- Satellites > 6 (sinyal kuat)

Contoh kode:
```cpp
if (gps.location.isValid() && 
    gps.speed.kmph() > 1.0 && 
    gps.satellites.value() > 6) {
    logGPSToSD(lat, lng);
}
```

## Fitur Tambahan yang Bisa Dikembangkan

1. **Auto-delete old files** - Hapus file lebih dari 30 hari
2. **KML/GPX export** - Format untuk Google Earth
3. **Trip summary** - Total jarak, durasi, kecepatan rata-rata
4. **Geofencing** - Alert saat keluar dari area tertentu
5. **Waypoint marking** - Simpan lokasi penting dengan tombol

## Pin Diagram (Ringkasan)

```
Heltec Tracker V1.1
├── GPS (UART)
│   ├── RX: GPIO 33
│   └── TX: GPIO 34
├── SD Card (SPI)
│   ├── CS:   GPIO 4  ← Pin penting untuk SD
│   ├── MOSI: GPIO 10 (shared dengan Radio)
│   ├── MISO: GPIO 11 (shared dengan Radio)
│   └── SCK:  GPIO 9  (shared dengan Radio)
└── TFT Display (SPI)
    ├── CS:   GPIO 38
    ├── DC:   GPIO 40
    ├── RST:  GPIO 39
    ├── MOSI: GPIO 42
    └── SCK:  GPIO 41
```

**Catatan:** SD Card menggunakan SPI bus yang sama dengan Radio LoRa, namun dengan CS pin yang berbeda (GPIO 4).
