# Panduan Lengkap Setup Laser Tag Project

Dokumen ini menjelaskan langkah-langkah menghubungkan device Laser Tag ke internet menggunakan berbagai metode koneksi.

## Daftar Isi

1. [Pilihan Cara Koneksi](#1-pilihan-cara-koneksi)
2. [Prioritas 1: LoRaWAN → TTN → Vercel](#2-prioritas-1-lorawan--ttn--vercel)
3. [Prioritas 2: GPRS → ThingSpeak → Vercel](#3-prioritas-2-gprs--thingspeak--vercel)
4. [Prioritas 3: WiFi → Vercel Langsung](#4-prioritas-3-wifi--vercel-langsung)
5. [Prioritas 4: SD Card Backup (GPS CSV Logger)](#5-prioritas-4-sd-card-backup-gps-csv-logger)
6. [Setup Vercel & NeonDB](#6-setup-vercel--neondb)
   6.6 [Memahami Kolom Dashboard](#66-memahami-kolom-dashboard)
7. [Troubleshooting](#7-troubleshooting)

---

## 1. Pilihan Cara Koneksi

Sebelum memulai, pilih metode koneksi yang sesuai dengan kebutuhan Anda:

| Prioritas | Cara | Kebutuhan Hardware | Biaya Bulanan | Keterangan |
|-----------|------|-------------------|---------------|------------|
| 1 | LoRaWAN → TTN | LoRa Gateway | Gratis | Jangkauan terbatas (dekat gateway), gratis pakai TTN |
| 2 | GPRS → ThingSpeak | SIM900A + SIM Card | Rp 10.000-30.000 | Nasional (jaringan seluler), butuh pulsa data |
| 3 | WiFi → Vercel | WiFi tersedia | Sesuai WiFi | Indoor/gedung, paling mudah |
| 4 | SD Card | SD Card saja | Gratis | Backup offline, tidak butuh internet |

### Diagram Alur Data

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        CARA KONEKSI LENGKAP                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  PRIORITAS 1: LoRaWAN → TTN → Vercel                                  │
│  ┌─────────┐    ┌──────────┐    ┌─────────┐    ┌──────────────────┐  │
│  │  ESP32  │───→│  LoRa    │───→│   TTN   │───→│  Vercel API     │  │
│  │ (Heltec)│    │ Gateway  │    │ Console │    │  /api/ttn       │  │
│  └─────────┘    └──────────┘    └─────────┘    └────────┬─────────┘  │
│                                                          │             │
│  PRIORITAS 2: GPRS → ThingSpeak → Vercel               │             │
│  ┌─────────┐    ┌──────────┐    ┌──────────┐   ┌──────┘             │
│  │  ESP32  │───→│ SIM900A  │───→│ThingSpeak│───┘                    │
│  │ (Heltec)│    │  (GPRS)  │    │ Channel │    ┌──────────────────┘  │
│  └─────────┘    └──────────┘    └────┬─────┘                        │
│                                       │                                │
│  PRIORITAS 3: WiFi → Vercel         │                                │
│  ┌─────────┐    ┌──────────┐         │                                │
│  │  ESP32  │───→│   WiFi   │─────────┘                                │
│  │ (Heltec)│    │  Router  │                                         │
│  └─────────┘    └──────────┘                                         │
│                                       │                                │
│  PRIORITAS 4: SD Card (Backup)      │                                │
│  ┌─────────┐    ┌──────────┐         │                                │
│  │  ESP32  │───→│ SD Card  │─────────┘                                │
│  │ (Heltec)│    │  /gps_log.csv │                                        │
│  └─────────┘    └──────────┘                                          │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Prioritas 1: LoRaWAN → TTN → Vercel

Cara ini menggunakan LoRaWAN untuk mengirim data ke TTN (The Things Network), lalu dari TTN dikirim ke Vercel.

### 2.1 Apa itu TTN?

TTN (The Things Network) adalah platform LoRaWAN gratis. Anda butuh:
- Akun TTN (pakai GitHub untuk login)
- LoRa Gateway (bisa pakai Heltec lain atau community gateway)
- Device yang sudah di-config dengan credentials TTN

### 2.2 Langkah-Langkah Setup TTN

#### Langkah 1: Daftar TTN

1. Buka browser, akses: **https://console.thethings.network**
2. Klik **"Log in with GitHub"** (paling mudah)
3. Jika muncul pop-up, klik **"Authorize"**
4. Selesai! Anda sudah punya akun TTN

#### Langkah 2: Buat Application

1. Setelah login, klik **"Go to Console"**
2. Klik **"Applications"** → **"Create Application"**
3. Isi form:

| Kolom | Isi | Contoh |
|-------|-----|--------|
| Application ID | Nama unik (huruf kecil, angka, tanda minus) | `laser-tag-tracker` |
| Application Name | Nama tampilan | `Laser Tag GPS Tracker` |
| Description | Deskripsi opsional | `GPS tracker untuk game laser tag` |

4. Klik **"Create Application"**

#### Langkah 3: Register End Device

1. Di halaman Application yang baru dibuat, klik **"Add end device"**
2. Di bagian **"Select device type"**, pilih:
   - **"Enter end device specifics manually"**
3. Di bagian **"Fill parameters"**, isi:

| Parameter | Nilai |
|-----------|-------|
| Frequency Plan | `Asia 920-923 MHz (AS923)` (paling cocok untuk Indonesia) |
| JoinEUI | Klik **"Generate"** untuk membuat baru |
| DevEUI | Klik **"Generate"** untuk membuat baru |
| AppKey | Klik **"Generate"** untuk membuat baru |

4. Klik **"Register device"**

#### Langkah 4: Simpan Credentials

**PENTING!** Setelah register device, simpan informasi berikut di tempat aman:

```
JoinEUI (AppEUI): 0000000000000003
DevEUI: 70B3D57ED0075AC0
AppKey: 48F03FDD9C5A9EBA4283011D7DBBF3F8
NwkKey: 9BF612F4AA33DB73AA1B7AC64D700226
```

> **Catatan:** Ini contoh credentials. Gunakan credentials yang Anda generate sendiri!

### 2.3 Setup Payload Formatter

Payload formatter diperlukan untuk menerjemahkan data biner dari device menjadi format JSON yang bisa dibaca.

1. Di TTN Console, klik **"Payload Formats"** → **"Uplink"**
2. Hapus kode yang ada
3. Copy dan paste kode berikut:

```javascript
function decodeUplink(input) {
    var bytes = input.bytes;
    
    if (bytes.length < 30) {
        return { data: { error: "Invalid payload: expected 30 bytes" }};
    }
    
    // Fungsi helper untuk baca float little-endian
    function readFloat32LE(b, o) {
        var buffer = new ArrayBuffer(4);
        var view = new DataView(buffer);
        for (var i = 0; i < 4; i++) {
            view.setUint8(i, b[o + i]);
        }
        return view.getFloat32(0, true);
    }
    
    // Fungsi helper untuk baca uint16 little-endian
    function readUInt16LE(b, o) {
        return b[o] | (b[o + 1] << 8);
    }
    
    // Fungsi helper untuk baca int16 little-endian (bisa negatif)
    function readInt16LE(b, o) {
        var val = b[o] | (b[o + 1] << 8);
        return val > 32767 ? val - 65536 : val;
    }
    
    // Parse data dari payload
    var address_id = readUInt16LE(bytes, 0);
    
    data.lat = readFloat32LE(bytes, 11);
    data.lng = readFloat32LE(bytes, 15);
    data.alt = readFloat32LE(bytes, 19);
    data.battery = readUInt16LE(bytes, 23);
    data.satellites = bytes[25];
    data.rssi = readInt16LE(bytes, 26);
    data.snr = bytes[28];
    data.cheatDetected = bytes[29];
    data.status = bytes[10];
    
    // Buat Device ID dari address_id
    var playerNum = (address_id & 0xFF) + 1;
    data.deviceId = "Heltec-P" + playerNum.toString();
    
    // IR Status
    data.irStatus = data.status === 1 ? "HIT" : "-";
    
    // Cheat Status
    data.cheatStatus = data.cheatDetected === 1 ? "CHEAT!" : "-";
    
    return {
        data: {
            deviceId: data.deviceId,
            lat: data.lat,
            lng: data.lng,
            alt: data.alt,
            irStatus: data.irStatus,
            cheatStatus: data.cheatStatus,
            battery: data.battery,
            satellites: data.satellites,
            rssi: data.rssi,
            snr: data.snr,
            cheatDetected: data.cheatDetected
        }
    };
}
```

4. Klik **"Save payload formatter"**

### 2.4 Setup Webhook ke Vercel

Webhook berfungsi untuk mengirim data dari TTN ke Vercel secara otomatis.

1. Di TTN Console, klik **"Integrations"** → **"Webhooks"**
2. Klik **"Add webhook"** → **"Custom webhook"**
3. Isi form:

| Field | Isi |
|-------|-----|
| Webhook ID | `vercel-gps-tracker` |
| Webhook URL | `https://laser-tag-project.vercel.app/api/ttn` |
| Method | `POST` |
| Uplink message | ✅ Centang (Enabled) |

4. Klik **"Add Webhook"**

### 2.5 Konfigurasi Firmware

Setelah TTN sudah disiapin, sekarang konfigurasi firmware di Arduino IDE.

1. Buka file firmware: `firmware/heltec_tracker_v1_5/heltec_tracker_v1_5.ino`
2. Cari bagian **LoRaWAN Credentials** (sekitar baris 85):

```cpp
// ============================================
// LORAWAN KEYS
// ============================================
const uint64_t joinEUI = 0x0000000000000003;  // GANTI dengan JoinEUI Anda
const uint64_t devEUI  = 0x70B3D57ED0075AC0;   // GANTI dengan DevEUI Anda

const uint8_t appKey[] = {
    0x48, 0xF0, 0x3F, 0xDD, 0x9C, 0x5A, 0x9E, 0xBA,
    0x42, 0x83, 0x01, 0x1D, 0x7D, 0xBB, 0xF3, 0xF8
};  // GANTI dengan AppKey Anda

const uint8_t nwkKey[] = {
    0x9B, 0xF6, 0x12, 0xF4, 0xAA, 0x33, 0xDB, 0x73,
    0xAA, 0x1B, 0x7A, 0xC6, 0x4D, 0x70, 0x02, 0x26
};  // GANTI dengan NwkKey Anda
```

3. Ganti dengan credentials yang Anda dapat dari TTN Console

4. Pastikan LoRaWAN diaktifkan:

```cpp
#define ENABLE_LORAWAN 1   // Pastikan 1 untuk mengaktifkan
```

5. **Upload** firmware ke ESP32 menggunakan Arduino IDE

### 2.6 Testing LoRaWAN

1. **Buka Serial Monitor** di Arduino IDE
   - Tools → Serial Monitor
   - Baud rate: **115200**

2. **Nyalakan device** (hubungkan power)

3. **Perhatikan output** Serial Monitor:
   - Seharusnya muncul: `[LoRa] Join: ATTEMPT 1/10`
   - Kemudian: `[LoRa] Join: SUCCESS` (jika berhasil)

4. **Buka TTN Console** → Application → **Live Data**
   - Jika berhasil, akan muncul uplink baru
   - Klik salah satu uplink untuk melihat decoded payload

### 2.7 Struktur Payload (30 bytes)

| Offset | Size | Type | Field | Penjelasan |
|--------|------|------|-------|------------|
| 0-3 | 4 | uint32 | address_id | ID player (0=P1, 1=P2, dst) |
| 4 | 1 | uint8 | sub_id | Sub ID |
| 5-8 | 4 | uint32 | shooter_addr | Alamat player yang menembak |
| 9 | 1 | uint8 | shooter_sub | Sub ID penembak |
| 10 | 1 | uint8 | status | 0=normal, 1=terkena tembakan |
| 11-14 | 4 | float | lat | Latitude GPS |
| 15-18 | 4 | float | lng | Longitude GPS |
| 19-22 | 4 | float | alt | Ketinggian (meter) |
| 23-24 | 2 | uint16 | battery | Battery (mV) |
| 25 | 1 | uint8 | sat | Jumlah satelit |
| 26-27 | 2 | int16 | rssi | Signal strength (dBm) |
| 28 | 1 | int8 | snr | Signal quality (dB) |
| 29 | 1 | uint8 | cheatDetected | 0=normal, 1=cheat |

---

## 3. Prioritas 2: GPRS → ThingSpeak → Vercel

Cara ini menggunakan GPRS (melalui modem SIM900A) untuk mengirim data ke ThingSpeak, lalu dari ThingSpeak ke Vercel.

### 3.1 Persiapan Hardware

#### Alat yang Dibutuhkan:
- **Modem SIM900A** - bisa beli online (Rp 150.000-300.000)
- **SIM Card** - pakai Indosat/XL/Telkomsel dengan paket data
- **Antena GSM** - biasanya sudahinclude sama SIM900A
- **Power Supply** - 5V 2A (SIM900A butuh arus cukup besar)

#### Sambungan Kabel:
```
Heltec Wireless Tracker → SIM900A
┌─────────────────────────────────────┐
│ Heltec Pin    →  SIM900A Pin       │
├─────────────────────────────────────┤
│ GPIO 16 (TX)  →  SIM900A RX        │
│ GPIO 17 (RX)  →  SIM900A TX        │
│ GPIO 15 (RST) →  SIM900A RST       │
│ VCC 5V        →  VCC               │
│ GND           →  GND               │
└─────────────────────────────────────┘
```

### 3.2 Setup ThingSpeak

#### Langkah 1: Buat Akun ThingSpeak

1. Buka browser, akses: **https://thingspeak.com**
2. Klik **"Sign Up"** (atau "Sign In" jika sudah punya)
3. Bisa login dengan:
   - MathWorks Account
   - Google
   - GitHub
4. Verifikasi email jika diperlukan

#### Langkah 2: Buat Channel Baru

1. Setelah login, klik **"Channels"** → **"My Channels"**
2. Klik **"New Channel"**
3. Isi detail channel:

| Kolom | Isi |
|-------|-----|
| Name | `Laser Tag GPS Tracker` |
| Description | `GPS tracker untuk game laser tag` |

4. Aktifkan **Field 1** sampai **Field 8**:

| Field | Nama Field | Deskripsi |
|-------|-----------|-----------|
| Field 1 | latitude | Latitude GPS |
| Field 2 | longitude | Longitude GPS |
| Field 3 | altitude | Ketinggian (meter) |
| Field 4 | satellites | Jumlah satelit |
| Field 5 | rssi | Signal strength |
| Field 6 | irStatus | Status IR (1=HIT, 0=NONE) |
| Field 7 | cheat_detected | Status cheat (0/1) |
| Field 8 | snr | Signal quality |

5. Klik **"Save Channel"**

#### Langkah 3: Catat API Key

1. Klik tab **"API Keys"**
2. Catat informasi berikut:

| Parameter | Contoh Nilai |
|-----------|--------------|
| Channel ID | `3268510` |
| Write API Key | `N6ATMD4JVI90HTHH` |

> **PENTING!** Simpan credentials ini di tempat aman.

### 3.3 Setup ThingHTTP (untuk Forward ke Vercel)

ThingHTTP berfungsi untuk forward data dari ThingSpeak ke Vercel.

1. Di ThingSpeak, klik **"Apps"** → **"ThingHTTP"**
2. Klik **"New ThingHTTP"**
3. Isi form:

| Field | Isi |
|-------|-----|
| Name | `ForwardToVercel` |
| URL | `https://laser-tag-project.vercel.app/api/track` |
| Method | `POST` |
| Content Type | `application/json` |

4. Di bagian **Body**, isi:

```json
{
  "source": "thingspeak",
  "deviceId": "Heltec-P1",
  "lat": %%channel_3268510_field_1%%,
  "lng": %%channel_3268510_field_2%%,
  "alt": %%channel_3268510_field_3%%,
  "sats": %%channel_3268510_field_4%%,
  "rssi": %%channel_3268510_field_5%%,
  "irStatus": %%channel_3268510_field_6%%,
  "cheatDetected": %%channel_3268510_field_7%%,
  "snr": %%channel_3268510_field_8%%
}
```

> **Catatan:** Ganti `3268510` dengan Channel ID Anda!

5. Klik **"Save ThingHTTP"**

### 3.4 Setup React (Trigger Otomatis)

React berfungsi untuk menjalankan ThingHTTP secara otomatis saat ada data masuk.

1. Di ThingSpeak, klik **"Apps"** → **"React"**
2. Klik **"New React"**
3. Isi form:

| Field | Isi |
|-------|-----|
| React Name | `TriggerVercel` |
| Condition Type | `Numeric` |
| Test Frequency | `on data insertion` |
| Condition | Field 4 `is greater than` `0` |
| Action | ThingHTTP → `ForwardToVercel` |

4. Klik **"Save React"**

### 3.5 Konfigurasi APN Operator

#### Daftar APN Operator Indonesia:

| Operator | APN | Username | Password |
|----------|-----|----------|----------|
| Indosat | `indosatgprs` | `indosat` | `indosat` |
| XL | `xlgr` | `xlgprs` | `xlgprs` |
| Telkomsel | `telkomsel` | (kosong) | (kosong) |
| Three | `3gprs` | `3gprs` | `3gprs` |

### 3.6 Konfigurasi Firmware (Bagian GPRS & ThingSpeak)

1. Buka file firmware: `firmware/heltec_tracker_v1_5/heltec_tracker_v1_5.ino`

2. **Konfigurasi GPRS** (baris ~44-53):

```cpp
// ============================================
// KONFIGURASI GPRS
// ============================================
#define GPRS_APN            "indosatgprs"    // Ganti sesuai operator
#define GPRS_USER           "indosat"        // Ganti sesuai operator
#define GPRS_PASS           "indosat"        // Ganti sesuai operator
#define GPRS_SIM_PIN        ""               // Kosongkan jika tidak pakai PIN

#define GPRS_HOST           "simpera.teknoklop.com"
#define GPRS_PORT           10000
#define GPRS_API_KEY        "tPmAT5Ab3j7F9"
#define GPRS_PATH           "/lasertag/api/track.php"
#define GPRS_CHEAT_PATH     "/lasertag/api/cheat_event.php"
```

3. **Konfigurasi ThingSpeak** (baris ~58-62):

```cpp
// ============================================
// THINGSPEAK VIA SIM900A
// ============================================
#define TS_HOST             "api.thingspeak.com"
#define TS_PORT             80
#define TS_WRITE_KEY        "N6ATMD4JVI90HTHH"   // Ganti dengan API Key Anda
#define TS_CHANNEL_ID       "3268510"            // Ganti dengan Channel ID Anda
#define TS_INTERVAL_MS      15000                // 15 detik (minimal ThingSpeak free)
```

4. Pastikan GPRS diaktifkan:

```cpp
#define ENABLE_GPRS    1   // Pastikan 1 untuk mengaktifkan
```

5. **Upload** firmware ke ESP32

### 3.7 Testing GPRS → ThingSpeak

1. **Buka Serial Monitor** (115200 baud)
2. **Nyalakan device**
3. Perhatikan output:
   - `[GPRS] Initializing GPRS modem...`
   - `[GPRS] Connecting to indosatgprs...`
   - `[GPRS] Connected persistently, IP=10.x.x.x`

4. Buka **ThingSpeak** → Channel Anda → **Private View**
5. Tunggu 15 detik, harusnya data GPS muncul di chart

### 3.8 Troubleshooting ThingSpeak

| Masalah | Penyebab | Solusi |
|--------|----------|--------|
| Data tidak masuk | GPRS tidak konek | Cek SIM Card, pulsa data, APN |
| Chart kosong | Interval belum tercapai | ThingSpeak free min 15 detik |
| HTTP error | API Key salah | Cek Channel ID dan Write API Key |
| React tidak trigger | Condition salah | Cek konfigurasi React |

---

## 4. Prioritas 3: WiFi → Vercel Langsung

Cara paling mudah! Hanya butuh WiFi tersedia.

### 4.1 Konfigurasi Firmware (WiFi)

1. Buka file firmware: `firmware/heltec_tracker_v1_5/heltec_tracker_v1_5.ino`

2. Cari bagian **WiFi Configuration** (baris ~41-43):

```cpp
// ============================================
// KONFIGURASI
// ============================================
#define WIFI_SSID           "NamaWiFiAnda"      // Ganti dengan nama WiFi
#define WIFI_PASSWORD       "PasswordWiFi"      // Ganti dengan password WiFi
#define VERCEL_API_URL      "https://laser-tag-project.vercel.app/api/track"
```

3. Pastikan WiFi diaktifkan:

```cpp
#define ENABLE_WIFI    1   // Pastikan 1 untuk mengaktifkan
```

4. **Upload** firmware

### 4.2 Testing WiFi

1. **Buka Serial Monitor** (115200 baud)
2. **Nyalakan device**
3. Perhatikan output:
   ```
   [WiFi] Connecting: NamaWiFiAnda
   [WiFi] Connected | IP: 192.168.1.100
   [WiFi] Upload: SUCCESS | Code: 200
   ```

4. Buka Vercel Dashboard untuk lihat data

---

## 5. Prioritas 4: SD Card Backup (GPS CSV Logger)

Firmware V1.5 memiliki fitur GPS CSV Logger yang otomatis menyimpan data GPS ke SD Card.

### 5.1 Fitur GPS CSV Logger

- **File**: `/gps_log.csv` di SD Card
- **Interval**: Setiap 5 detik
- **Persist**: Data TIDAK dihapus saat reboot (mode append)
- **Offline**: Tidak butuh internet

### 5.2 Struktur Data CSV

| Kolom | Deskripsi |
|-------|-----------|
| millis | Waktu sejak boot (milidetik) |
| jam | Jam (sesuai timezone Indonesia) |
| menit | Menit |
| detik | Detik |
| latitude | Latitude GPS |
| longitude | Longitude GPS |
| altitude_m | Ketinggian (meter) |
| satellites | Jumlah satelit terkunci |
| gps_valid | 1=valid, 0=tidak valid |

### 5.3 Contoh Data CSV

```csv
millis,jam,menit,detik,latitude,longitude,altitude_m,satellites,gps_valid
5000,10,30,45,-6.208800,106.845600,45.5,8,1
10000,10,30,50,-6.208900,106.845700,46.0,8,1
```

### 5.4 Cara Mengambil Data CSV

1. **Matikan device** (putus power)
2. **Cabut SD Card** dari slot
3. **Pasang SD Card** ke card reader di komputer
4. **Buka file** `/gps_log.csv` dengan Excel/Google Sheets
5. Import data untuk analisis

### 5.5 Konfigurasi (Opsional)

Jika ingin ubah interval atau nama file:

```cpp
// ============================================
// GPS CSV CONFIG
// ============================================
#define GPS_CSV_FILE        "/gps_log.csv"
#define GPS_CSV_INTERVAL_MS 5000   // 5 detik
```

---

## 6. Setup Vercel & NeonDB

Bagian ini menjelaskan cara setup Vercel dan NeonDB untuk menyimpan data.

### 6.1 Buat Akun Vercel

1. Buka: **https://vercel.com**
2. Klik **"Sign Up"** → **"Continue with GitHub"**
3. Authorize Vercel

### 6.2 Import Project dari GitHub

1. Klik **"Add New..."** → **"Project"**
2. Cari repository: `latifannurdiansyah/Laser-tag-project`
3. Klik **"Import"**

### 6.3 Setup Environment Variables

1. Di bagian **"Environment Variables"**, klik **"Add"**
2. Isi:

| Name | Value |
|------|-------|
| DATABASE_URL | (lihat langkah berikutnya) |

3. Klik **"Save"**
4. Klik **"Deploy"**

### 6.4 Setup NeonDB

1. Buka: **https://console.neon.tech**
2. Klik **"Create Project"**
3. Isi:

| Kolom | Isi |
|-------|-----|
| Project Name | `laser-tag-db` |
| Database Name | `neondb` |
| Region | `Asia Pacific (Singapore)` |

4. Klik **"Create Project"**

5. Setelah dibuat, klik **"Connection Details"**
6. Copy **Connection String** (format: `postgresql://...`)

7. Paste ke Vercel Environment Variables:

| Name | Value |
|------|-------|
| DATABASE_URL | `postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require` |

8. Di Vercel, klik **"Deploy"** lagi

### 6.5 Cek Data di Dashboard

Setelah semua terhubung:

- **Dashboard**: `https://laser-tag-project.vercel.app/dashboard`
- **API Track**: `https://laser-tag-project.vercel.app/api/track`

### 6.6 Memahami Kolom Dashboard

Dashboard menampilkan data dengan kolom-kolom berikut:

| Kolom | Deskripsi | Contoh |
|-------|-----------|--------|
| ID | Nomor urut data | 1, 2, 3... |
| SRC | Sumber data | `wifi` (WiFi HTTP), `ttn` (LoRaWAN), `thingspeak` |
| DEVICE | ID device | Heltec-P1 |
| LATITUDE | Latitude GPS | -6.208800 |
| LONGITUDE | Longitude GPS | 106.845600 |
| ALT (m) | Ketinggian (meter) | 45.5 |
| SAT | Jumlah satelit | 8 |
| RSSI | Signal strength | -90 dBm |
| SNR | Signal quality | 7.5 dB |
| CHEAT | Status cheat | **TERDETEKSI** (merah) / **-** (kosong) |
| STATUS | Status IR | HIT (merah) / - (normal) |
| TIMESTAMP | Waktu data masuk | 2025-01-01 10:30:45 |

**Kolom CHEAT:**
- **TERDETEKSI** = Sensor IR sensor tertutup >5 detik (indikasi cheat)
- **-** = Normal (tidak ada cheat)

**Kolom STATUS:**
- **HIT** = Player tertembak (sensor IR menerima sinyal)
- **-** = Normal

---

## 7. Troubleshooting

### 7.1 Troubleshooting LoRaWAN/TTN

| Masalah | Penyebab | Solusi |
|--------|----------|--------|
| Join never succeeds | Credentials salah | Cek DevEUI, JoinEUI, AppKey |
| Join lambat | Gateway jauh | Dekatkan device ke gateway |
| No uplink | Frequency plan salah | Pastikan AS923 untuk Indonesia |
| Webhook tidak work | URL salah | Cek Webhook URL di TTN Console |

### 7.2 Troubleshooting GPRS

| Masalah | Solusi |
|---------|--------|
| Modem tidak respons | Cek wiring RX/TX, pastikan 5V power cukup |
| SIM tidak terdeteksi | Cek SIM card terpasang dengan benar |
| GPRS connect gagal | Cek APN benar, cek pulsa/data tersedia |
| TCP connect gagal | Cek host/port, cek firewall |
| Data tidak sampai | Cek server endpoint berjalan |

### 7.3 Troubleshooting WiFi

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| WiFi tidak konek | SSID/password salah | Cek SSID dan password benar |
| Upload gagal | URL Vercel salah | Cek URL Vercel benar |
| Sering disconnect | Signal lemah | Cek signal WiFi kuat |
| WiFi gagal (code:-1) saat GPRS aktif | Routing conflict | WiFi hanya jika GPRS OFF |

### 7.4 Troubleshooting WiFi + GPRS Bersama

Ketika kedua interface (WiFi dan GPRS) aktif bersamaan, kadang terjadi konflik routing:

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| WiFi: FAIL (code:-1) | ESP32 routing salah | Firmware priority: GPRS, skip WiFi jika GPRS ON |
| WiFi tidak bisa HTTPS | Default route ke GPRS | Gunakan salah satu saja |

**Catatan**: Firmware V1.5 mengirim data via WiFi hanya jika GPRS tidak terhubung. Ini adalah desain untuk menghindari konflik routing.

### 7.5 Troubleshooting SD Card

| Masalah | Solusi |
|---------|--------|
| SD tidak terdeteksi | Cek wiring, pastikan CS pin benar |
| File tidak tercipta | Cek SD Card tidak write-protected |
| Data korup | Cek power supply cukup |

### 7.6 Serial Monitor Output Reference

Firmware V1.5 menampilkan berbagai informasi di Serial Monitor:

```
========================================
Heltec Tracker V1.5
Device ID: Heltec-P1
========================================

[SD] SDcard = Connected 1536Mb    ← SD Card OK
[WiFi] Connecting: UserAndroid    ← Mencoba konek WiFi
[WiFi] Connected | IP: 192.168.1.100  ← WiFi OK
[WiFi] Upload: SUCCESS | Code: 200  ← Upload berhasil
[LoRa] Join: ATTEMPT 1/10        ← Mencoba join TTN
[LoRa] Join: SUCCESS              ← Join berhasil
[LoRa] TX: SUCCESS | RSSI: -75 | SNR: 7.5  ← TX berhasil
[GPRS] Initializing GPRS modem...  ← Memulai GPRS
[GPRS] Connected persistently, IP=10.x.x.x  ← GPRS OK
[SCHEDULED GPRS] DB_OK            ← Kirim data terjadwal berhasil
[IMMEDIATE GPRS] IR_DB_OK         ← Kirim IR hit berhasil
[CHEAT] DETECTED!                 ← Cheat terdeteksi
[TS] OK code:200 lat:-6.2088      ← ThingSpeak OK
```

---

## Versi Firmware

| Versi | Tanggal | Perubahan |
|-------|---------|-----------|
| V1.5 | Terbaru | + GPS CSV Logger, irStatus field6, SD status bar |
| V1.4 | - | + ThingSpeak via GPRS |
| V1.3 | - | + Anti-Cheat System |
| V1.2 | - | + WiFi + GPRS backup |
| V1.1 | - | + LoRaWAN + IR |
| V1.0 | - | Base GPS + TFT |

---

## Catatan Penting

1. **Pilih salah satu cara** - Jangan aktifkan semua sekaligus, cukup pilih yang sesuai kebutuhan
2. **Backup credentials** - Simpan semua API Key, password, dan credentials di tempat aman
3. **Testing** - Selalu test dengan Serial Monitor sebelum deploy lapangan
4. **Power** - Pastikan power supply cukup untuk semua modul
5. **Antena** - Gunakan anten yang tepat untuk GPS dan LoRa

---

## Lisensi

Project ini dibuat untuk tugas akhir/skripsi.
