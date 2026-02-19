# Laser Tag Project - Complete Setup Guide

## Daftar Isi

0. [Persiapan Awal - Dari Nol](#0-persiapan-awal---dari-nol)
   - [0.1 Buat GitHub Account](#01-buat-github-account)
   - [0.2 Install Git di Windows](#02-install-git-di-windows)
   - [0.3 Clone Project dari GitHub](#03-clone-project-dari-github)
   - [0.4 Buat Vercel Account](#04-buat-vercel-account)
1. [NeonDB → Vercel Connection](#1-neondb--vercel-connection)
2. [TTN Integration](#2-ttn-integration)
   - [Compact Payload Format (Optional)](#26a-compact-payload-format-optional)
   - [Serial Monitor Debug Output](#27a-serial-monitor-debug-output)
   - [Troubleshooting TTN](#28-troubleshooting-ttn)
   - [Troubleshooting ESP32 Restart](#29-troubleshooting-esp32-restart)
   - [SD Card Configuration](#210-sd-card-configuration)
3. [TFT Display 3 Pages Scrolling](#3-tft-display-3-pages-scrolling)

---

## 0. Persiapan Awal - Dari Nol

 Bagian ini menjelaskan langkah-langkah dari awal sekali: buat GitHub, install Git, clone project, dan deploy ke Vercel. Ikuti semua langkah di bawah ini secara berurutan.

### 0.1 Buat GitHub Account

GitHub adalah platform untuk menyimpan dan mengelola kode project. Ikuti langkah berikut untuk membuat akun:

#### Langkah-langkah:

1. **Buka website GitHub**
   - Buka browser (Chrome/Edge/Firefox)
   - Kunjungi: **https://github.com**

2. **Klik "Sign Up"**
   - Di pojok kanan atas, klik tombol **"Sign Up"**

3. **Isi formulir pendaftaran**
   - **Email**: Masukkan email aktif Anda
   - **Password**: Minimal 8 karakter
   - **Username**: Nama unik untuk identitas Anda (misal: `latifannurdiansyah`)
   - Klik **"Continue"**

4. **Verifikasi email**
   - GitHub akan mengirim kode ke email Anda
   - Masukkan kode verifikasi tersebut

5. **Pilih plan** (gratis)
   - Pilih **"Join a free plan"**
   - Klik **"Continue"**

6. **Selesai!**
   - Akun GitHub Anda sudah siap
   - Simpan username dan password dengan aman

#### Catatan:
- **GitHub Free**: Semua fitur dasar gratis, termasuk repository publik dan privat
- Akun ini akan digunakan untuk login ke Vercel juga

---

### 0.2 Install Git di Windows

Git adalah sistem versioning untuk mengelola kode. Ikuti langkah berikut:

#### Langkah-langkah:

1. **Download Git**
   - Buka: **https://git-scm.com/download/win**
   - Klik download untuk Windows (64-bit)

2. **Install Git**
   - Buka file installer yang sudah download
   - Klik **"Next"** terus sampai selesai
   - Pada pilihan **"Adjusting your PATH environment"**, pilih: **"Git from the command line and also from 3rd-party software"**
   - Klik **"Next"** → **"Next"** → **"Install"**
   - Tunggu sampai selesai, klik **"Finish"**

3. **Verifikasi Git terinstall**
   - Buka **Command Prompt** (ketik `cmd` di Start Menu)
   - Ketik:
   ```bash
   git --version
   ```
   - Jika muncul versi Git (misal: `git version 2.40.0`), berarti berhasil!

#### Alternatif: Git Bash
- Setelah install, klik kanan di folder mana saja → akan muncul **"Git Bash Here"**
- Git Bash lebih方便 dari Command Prompt

---

### 0.3 Clone Project dari GitHub

Clone berarti menyalin project dari GitHub ke komputer lokal Anda.

#### Langkah-langkah:

1. **Buka folder untuk project**
   - Buka **File Explorer**
   - Buat folder baru di: `C:\Users\aldi\Documents\Github_Latipan`
   - Jika folder sudah ada, lewati langkah ini

2. **Buka terminal (Git Bash atau Command Prompt)**
   - Jika pakai Git Bash: klik kanan di folder `Github_Latipan` → **"Git Bash Here"**
   - Atau buka Command Prompt, lalu:
   ```bash
   cd C:\Users\aldi\Documents\Github_Latipan
   ```

3. **Clone repository**
   - Ketik perintah ini:
   ```bash
   git clone https://github.com/latifannurdiansyah/Laser-tag-project.git
   ```
   - Tunggu sampai selesai (akan download semua file project)

4. **Masuk ke folder project**
   ```bash
   cd Laser-tag-project
   ```

5. **Verifikasi isi folder**
   - Ketik: `ls` (untuk lihat daftar file)
   - Anda akan melihat folder: `firmware/`, `src/`, `docs/`, `my-gps-tracker/`

#### Catatan:
- Anda hanya perlu clone SEKALI saja
- Untuk update project later, cukup `git pull`
- Jika ingin push perubahan ke GitHub, butuh konfigurasi SSH atau token

---

### 0.4 Buat Vercel Account

Vercel adalah platform hosting gratis untuk project Next.js. Ikuti langkah berikut:

#### Langkah-langkah:

1. **Buka website Vercel**
   - Buka browser
   - Kunjungi: **https://vercel.com**

2. **Daftar dengan GitHub**
   - Klik **"Sign Up"**
   - Pilih **"Continue with GitHub"**
   - Jika muncul pop-up, klik **"Authorize vercel"**

3. **Isi informasi tambahan**
   - Nama: Isi nama Anda
   - Team Name: Bisa dikosongkan atau isi nama bebas
   - Klik **"Continue"**

4. **Import repository**
   - Setelah login, klik **"Add New..."** → **"Project"**
   - Di bagian **"Import Git Repository"**, cari: `latifannurdiansyah/Laser-tag-project`
   - Klik **"Import"**

5. **Setup Environment Variables**
   - Cari bagian **"Environment Variables"**
   - Klik **"Add**"
   - Isi:
     - **Name**: `DATABASE_URL`
     - **Value**: (akan dapat dari NeonDB, lihat bagian 1.2)
   - Klik **"Save"**

6. **Deploy**
   - Klik **"Deploy"**
   - Tunggu 1-2 menit untuk build
   - Jika berhasil, akan muncul link seperti: `https://mygps-tracker.vercel.app`

7. **Catat URL-nya!**
   - Dashboard: `https://mygps-tracker.vercel.app/dashboard`
   - API: `https://mygps-tracker.vercel.app/api/track`

---

#### Cara Menjadikan Public (Semua Orang Bisa Akses)

**Vercel Defaultnya adalah PUBLIC!** 

Artinya:
- Semua orang bisa akses link dashboard Anda
- Tidak perlu login untuk lihat data
- Ini sudah sesuai untuk project laser tag Anda

Jika ingin ubah ke Private:
- Buka **Vercel Dashboard** → Project → **Settings**
- Scroll ke **"Git"** → matikan **"Make project public"**

---

## 0. Setup my-gps-tracker Project

### 0.1 Clone dari GitHub (Jika Baru)

Jika Anda belum punya folder `my-gps-tracker`, clone dari GitHub:

```bash
# Buka terminal/command prompt
cd C:\Users\aldi\Documents\Github_Latipan

# Clone repository
git clone https://github.com/latifannurdiansyah/Laser-tag-project.git

# Masuk ke folder project
cd Laser-tag-project
```

### 0.2 Setup dari Awal (Jika Ingin Buat Baru)

Jika ingin buat project Next.js baru dari nol:

```bash
# Buat project Next.js baru
npx create-next-app@latest my-gps-tracker

# Masuk ke folder
cd my-gps-tracker

# Install dependencies
npm install

# Install Prisma
npm install prisma @prisma/client
npx prisma init
```

### 0.3 File yang Harus Ada di my-gps-tracker/

```
my-gps-tracker/
├── app/
│   ├── api/
│   │   ├── logs/route.ts
│   │   ├── random/route.ts
│   │   ├── track/route.ts
│   │   └── ttn/route.ts
│   ├── dashboard/
│   │   └── page.tsx
│   ├── layout.tsx
│   └── page.tsx
├── lib/
│   └── prisma.ts
├── prisma/
│   └── schema.prisma
├── .env
├── package.json
├── tsconfig.json
└── next.config.ts
```

### 0.4 Install Dependencies

```bash
cd my-gps-tracker
npm install
```

### 0.5 Generate Prisma Client

```bash
npx prisma generate
```

### 0.6 Setup Environment Variables

Buat file `.env` di folder `my-gps-tracker/`:

```env
DATABASE_URL="postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require"
```

### 0.7 Push Schema ke Database

```bash
npx prisma db push
```

### 0.8 Run Development Server

```bash
npm run dev
```

Buka http://localhost:3000 untuk test lokal.

---

## 1. NeonDB → Vercel Connection

### 1.1 Buat Project di Neon

1. Buka **https://console.neon.tech**
2. Klik **"Sign Up"** atau **"Log In"** (pakai GitHub/email)
3. Klik **"Create Project"**
4. Isi detail:
   - **Project Name**: `laser-tag-db`
   - **Database Name**: `neondb`
   - **Region**: `Asia Pacific (Singapore)` (paling dekat dengan Indonesia)
5. Klik **"Create Project"**

### 1.2 Get Connection String

1. Setelah project dibuat, klik tab **"Connection Details"**
2. Cari bagian **"Connection String"**
3. Copy string yang显示 (format seperti ini):

```bash
postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require
```

4. **Simpan dengan aman** - ini adalah credentials database Anda

### 1.3 Setup di Vercel

1. Buka **https://vercel.com/dashboard**
2. Klik **"Add New..."** → **"Project"**
3. Import GitHub repo: `latifannurdiansyah/Laser-tag-project`
4. Klik **"Environment Variables"** tab
5. Klik **"Add"** dan isi:
   - **Name**: `DATABASE_URL`
   - **Value**: `postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require`
6. Klik **"Save"**
7. Klik **"Deploy"**

### 1.4 Setup Prisma di Local

1. Buka VS Code, buka folder `my-gps-tracker`
2. Buat file `.env`:

```env
DATABASE_URL="postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require"
```

3. Generate Prisma client:

```bash
cd my-gps-tracker
npx prisma generate
```

4. Push schema ke database:

```bash
npx prisma db push
```

5. Verifikasi - cek apakah tabel sudah ada:

```bash
npx prisma studio
```

### 1.5 Deploy ke Vercel (Production)

**Cara 1: Via GitHub Auto-deploy**
- Setiap kali push ke GitHub, Vercel auto-deploy
- Pastikan branch `main` aktif di Vercel Settings
- Pastikan folder root project di GitHub adalah `Laser-tag-project/`

**Cara 2: Via CLI**

```bash
cd my-gps-tracker

# Install Vercel CLI
npm i -g vercel

# Login
vercel login

# Pull environment dari Vercel
vercel env pull .env.production.local

# Deploy
npx prisma generate
npx prisma db push
vercel --prod
```

### 1.6 Troubleshooting NeonDB

| Masalah | Solusi |
|---------|--------|
| Connection timeout | Pastikan IP Anda di-allow di Neon Security settings |
| SSL error | Tambahkan `?sslmode=require` di connection string |
| Password salah | Reset password di Neon Dashboard → Settings → Password |
| Too many connections | Neon Free tier: max 100 connections |

---

## 2. TTN Integration

### 2.1 Buat Account TTN

1. Buka **https://console.thethings.network**
2. Klik **"Log in with GitHub"** (paling mudah)
3. Authorize aplikasi TTN

### 2.2 Buat Application

1. Klik **"Create Application"**
2. Isi detail:
   - **Application ID**: `laser-tag-tracker` (unik, lowercase + dash)
   - **Application name**: `Laser Tag GPS Tracker`
   - **Description**: `GPS tracker untuk game laser tag dengan LoRaWAN`
3. Klik **"Create Application"**

### 2.3 Register End Device

1. Di halaman Application, klik **"Add end device"**
2. Pilih tab **"Enter end device specifics manually"**
3. Isi:

```
Brand:            (Pilih "Heltec" jika ada, atau "Generic")
Model:            (Pilih ESP32 atau generic)
Hardware Version: 1.0
Firmware Version: LoRaWAN 1.0.3
Regional Parameters Version: RP001 Regional Parameters 1.0.3 (untuk AS923 - Indonesia)
```

4. Klik **"Start"**

5. **Input Device IDs**:
   - **Frequency plan**: `Asia 920-923 MHz (AS923)`
   - **DevEUI**: Klik **"Generate"** atau input sendiri (8 byte hex)
   - **AppEUI/JoinEUI**: Klik **"Generate"** (8 byte hex)
   - **AppKey**: Klik **"Generate"** (16 byte hex)

6. Klik **"Register device"**

7. **SIMPAN INFORMASI INI** (simpan di tempat aman):

```
DevEUI:     70B3D57ED0075AC0
JoinEUI:    0000000000000003
AppKey:     48F03FDD9C5A9EBA4283011D7DBBF3F8
NwkKey:     9BF612F4AA33DB73AA1B7AC64D700226
```

### 2.4 Configure Webhook ke Vercel

1. Di TTN Console, klik **"Integrations"** → **"Webhooks"**
2. Klik **"Add webhook"** → **"Custom webhook"**
3. Isi:
   ```
   Webhook ID:       vercel-gps-tracker
   Webhook URL:      https://mygps-tracker.vercel.app/api/ttn
   Method:           POST
   Uplink message:   ✅ Enabled (centang)
   ```
4. Klik **"Add Webhook"**

### 2.5 Setup Payload Formatter

1. Di TTN Console, klik **"Payload Formats"** → **"Uplink"**
2. Replace dengan kode ini:

```javascript
function decodeUplink(input) {
    var bytes = input.bytes;
    
    if (bytes.length < 29) {
        return { data: { error: "Invalid payload: expected 29 bytes" }};
    }
    
    // Little-endian float conversion (untuk ESP32)
    function readFloat32LE(b, o) {
        var buffer = new ArrayBuffer(4);
        var view = new DataView(buffer);
        for (var i = 0; i < 4; i++) {
            view.setUint8(i, b[o + i]);
        }
        return view.getFloat32(0, true); // true = little-endian
    }
    
    // Little-endian UInt16 (unsigned 16-bit)
    function readUInt16LE(b, o) {
        return b[o] | (b[o + 1] << 8);
    }
    
    // Little-endian Int16 (signed 16-bit) - untuk RSSI bisa negatif
    function readInt16LE(b, o) {
        var val = b[o] | (b[o + 1] << 8);
        return val > 32767 ? val - 65536 : val;
    }
    
    // Parse data
    var address_id = readUInt16LE(bytes, 0);  // uint32, tapi cukup uint16 untuk player ID
    
    data.lat = readFloat32LE(bytes, 11);
    data.lng = readFloat32LE(bytes, 15);
    data.alt = readFloat32LE(bytes, 19);
    
    // PERBAIKAN: Battery - little-endian (bytes 23-24)
    data.battery = readUInt16LE(bytes, 23);
    
    // Satellites (byte 25)
    data.satellites = bytes[25];
    
    // RSSI (bytes 26-27) - little-endian, signed int16
    data.rssi = readInt16LE(bytes, 26);
    
    // SNR (byte 28) - signed int8
    data.snr = bytes[28];
    
    // Status IR (byte 10)
    data.status = bytes[10];
    
    // Device ID: Heltec-P1, Heltec-P2, Heltec-P3... (based on address_id)
    var playerNum = (address_id & 0xFF) + 1;
    data.deviceId = "Heltec-P" + playerNum.toString();
    
    // IR Status: "-" jika tidak ada hit, "HIT" jika ada hit
    data.irStatus = data.status === 1 ? "HIT" : "-";
    
    return {
        data: {
            deviceId: data.deviceId,
            lat: data.lat,
            lng: data.lng,
            alt: data.alt,
            irStatus: data.irStatus,
            battery: data.battery,
            satellites: data.satellites,
            rssi: data.rssi,
            snr: data.snr
        }
    };
}
```

### 2.5a PENTING: Penjelasan Offset Payload

Struktur payload dari firmware (29 bytes total):

| Offset | Size | Type | Field | Contoh Nilai |
|--------|------|------|-------|--------------|
| 0-3 | 4 | uint32 | address_id | 0x00000000 |
| 4 | 1 | uint8 | sub_address_id | 0xFF |
| 5-8 | 4 | uint32 | shooter_address_id | 0xABCD1234 |
| 9 | 1 | uint8 | shooter_sub_address_id | 0x01 |
| 10 | 1 | uint8 | **status** (0=normal, 1=hit) | 0 atau 1 |
| 11-14 | 4 | float | **lat** (latitude) | -6.208800 |
| 15-18 | 4 | float | **lng** (longitude) | 106.845600 |
| 19-22 | 4 | float | **alt** (altitude) | 45.5 |
| 23-24 | 2 | uint16 | **battery** (mV) | 4200 |
| 25 | 1 | uint8 | **satellites** | 8 |
| 26-27 | 2 | int16 | **rssi** (dBm) | -75 |
| 28 | 1 | int8 | **snr** (dB) | 7 |

**CATATAN PENTING:**
- Semua dataGPS (lat, lng, alt) menggunakan format **float IEEE 754 little-endian**
- Battery dalam **millivolts** (mV), bukan Volt
- RSSI selalu **negatif** (misal: -75 dBm)
- Decoder ini WAJIB menggunakan little-endian karena ESP32 mengirim data dalam format little-endian

### 2.5b Cara Verifikasi Payload Decoder

Setelah menyimpan decoder, lakukan langkah berikut:

1. **Buka TTN Console** → Application → **Live Data**
2. **Nyalakan device** (ESP32 dengan firmware)
3. **Tunggu uplink masuk** - biasanya 30 detik sekali
4. **Klik salah satu uplink** untuk melihat decoded payload
5. **Verifikasi** decoded payload harus seperti ini:

```json
{
  "deviceId": "Heltec-P1",
  "lat": -6.208800,
  "lng": 106.845600,
  "alt": 45.5,
  "irStatus": "-",
  "battery": 4200,
  "satellites": 8,
  "rssi": -75,
  "snr": 7
}
```

**Jika decoded payload tidak muncul atau nilainya 0/null:**
- Cek apakah decoder sudah di-save dengan benar
- Cek ukuran payload di "Raw" - harus 29 bytes
- Lihat console browser untuk error message

### 2.5c Troubleshooting TTN Payload

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| lat/lng = 0 atau sangat besar | Offset byte salah atau endianness salah | Pastikan pakai little-endian |
| battery = 0 atau aneh | Byte order swapped | Pakai `bytes[o] \| (bytes[o+1] << 8)` |
| rssi selalu 0 | Format bukan signed int16 | Pakai readInt16LE dengan konversi negatif |
| satellites tidak muncul | Byte 25 tidak dibaca | Pastikan `bytes[25]` dibaca |
| snr tidak muncul | Byte 28 tidak dibaca | Pastikan `bytes[28]` dibaca |

---

### 2.6a Compact Payload Format (Optional - 9 bytes)

Jika ingin menggunakan payload yang lebih kecil (9 bytes) untuk efisiensi bandwidth:

#### Binary Payload Structure (9 bytes)

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | int32 | lat | Latitude × 1,000,000 |
| 4 | 4 | int32 | lng | Longitude × 1,000,000 |
| 8 | 1 | uint8 | ir_status | 0=no hit, 1=hit |

#### TTN Payload Decoder (Compact Format)

```javascript
function decodeUplink(input) {
  var bytes = input.bytes;
  
  if (bytes.length < 9) {
    return {
      data: { decoded: false, error: "Invalid payload" }
    };
  }
  
  // Decode 4 bytes big-endian signed int32 untuk latitude
  var latBits = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
  var latSigned = latBits >>> 0;
  if (latSigned >= 2147483648) latSigned = latSigned - 4294967296;
  var latitude = latSigned / 1000000;
  
  // Decode 4 bytes big-endian signed int32 untuk longitude
  var lngBits = (bytes[4] << 24) | (bytes[5] << 16) | (bytes[6] << 8) | bytes[7];
  var lngSigned = lngBits >>> 0;
  if (lngSigned >= 2147483648) lngSigned = lngSigned - 4294967296;
  var longitude = lngSigned / 1000000;
  
  // IR status (1 byte)
  var irStatus = bytes[8];
  
  return {
    data: {
      decoded: true,
      latitude: latitude,
      longitude: longitude,
      ir_status: irStatus,
      deviceId: "Heltec-P1"
    },
    warnings: [],
    errors: []
  };
}
```

> **Catatan:** Payload compact tidak termasuk altitude, battery, satellites, RSSI, dan SNR. Gunakan format lengkap (32 bytes) jika data tersebut dibutuhkan.

### Device ID Berdasarkan address_id

| address_id (di firmware) | Device ID (di dashboard) |
|---------------------------|---------------------------|
| `0x00000000` | Heltec-P1 |
| `0x00000001` | Heltec-P2 |
| `0x00000002` | Heltec-P3 |

### 2.6 Configure Firmware

1. Buka file:
   ```
   firmware/heltec-ir-gps-lora-playloadttn-vercel/heltec-ir-gps-lora-playloadttn-vercel.ino
   ```

2. Cari bagian **LoRaWAN Credentials** (baris ~34):

```cpp
const uint64_t joinEUI = 0x0000000000000003;  // GANTI DENGAN JoinEUI Anda
const uint64_t devEUI = 0x70B3D57ED0075AC0;   // GANTI DENGAN DevEUI Anda

const uint8_t appKey[] = {
    0x48, 0xF0, 0x3F, 0xDD, 0x9C, 0x5A, 0x9E, 0xBA,
    0x42, 0x83, 0x01, 0x1D, 0x7D, 0xBB, 0xF3, 0xF8
};

const uint8_t nwkKey[] = {
    0x9B, 0xF6, 0x12, 0xF4, 0xAA, 0x33, 0xDB, 0x73,
    0xAA, 0x1B, 0x7A, 0xC6, 0x4D, 0x70, 0x02, 0x26
};
```

3. Replace dengan credentials dari TTN Console Anda

4. **Upload firmware** ke ESP32 via Arduino IDE

### 2.7 Test Uplink

1. Power on device ESP32
2. Di TTN Console, klik **"Live data"** tab
3. Jika berhasil, Anda akan melihat uplink masuk:

```
Time    | Gatew | DevAddr | FCnt | Payload
10:30:45| gw1   | 01A2B3C4| 1    | 12345678...
```

4. Cek Vercel dashboard: `https://mygps-tracker.vercel.app/dashboard`
5. Data harus muncul dalam 30 detik (uplink interval)

### 2.7a Serial Monitor Debug Output

Firmware menampilkan output compact di Serial Monitor (115200 baud):

```
========================================
GPS Tracker + IR + LoRaWAN + WiFi + SD
Device ID: Heltec-P1
========================================

[SD] Connected | Size: 1536MB
[WiFi] Connecting: UserAndroid
[WiFi] Connected | IP: 192.168.1.100
[WiFi] Upload: SUCCESS | Code: 200
[LoRa] Join: ATTEMPT 1/10
[LoRa] Join: SUCCESS
[LoRa] TX: SUCCESS | RSSI: -120 | SNR: 7.5
```

#### Format Output

| Prefix | Arti |
|--------|------|
| `[SD] Connected \| Size: XMB` | SD Card berhasil mount |
| `[SD] Failed` | SD Card gagal mount |
| `[WiFi] Connecting: SSID` | Sedang konek WiFi |
| `[WiFi] Connected \| IP: X.X.X.X` | WiFi berhasil konek |
| `[WiFi] Upload: SUCCESS \| Code: 200` | Data berhasil upload |
| `[WiFi] Upload: FAILED \| Code: 500` | Gagal upload |
| `[WiFi] Reconnecting...` | WiFi terputus, mencoba reconnect |
| `[LoRa] Join: ATTEMPT X/10` | Proses join ke TTN |
| `[LoRa] Join: SUCCESS` | Join berhasil |
| `[LoRa] Join: FAILED` | Join gagal (akan retry) |
| `[LoRa] Join: MAX ATTEMPTS` | Gagal setelah 10x percobaan |
| `[LoRa] TX: SUCCESS \| RSSI: X \| SNR: Y.X` | Data berhasil dikirim |
| `[LoRa] TX: FAILED` | Gagal mengirim data |

#### Buka Serial Monitor

1. Di Arduino IDE: **Tools** → **Serial Monitor**
2. Set baud rate: **115200**
3. Centang: **Show timestamp** (opsional)

### 2.8 Troubleshooting TTN

| Masalah | Solusi |
|---------|--------|
| Join never succeeds | Cek DevEUI/JoinEUI format (harus LSB/hex) |
| No uplink received | Cek frequency plan (AS923 untuk Indonesia) |
| Webhook tidak trigger | Cek TTN → Integration → Webhook → "Recent deliveries" |
| Payload decode error | Cek payload formatter, pastikan format biner benar |

### 2.9 Troubleshooting ESP32 Restart

| Masalah | Solusi |
|---------|--------|
| Restart terus setelah SD Card connected | Tambah delay 500ms sebelum SD.begin(), gunakan sdInitialized flag |
| Restart saat GPS lock | Tunggu outdoor 2-5 menit untuk cold start |
| Restart saat LoRa TX | Kurangi payload size, tambahkan error handling |
| Guru Meditation Error | Cek stack overflow, tambah stack size task |

### 2.10 SD Card Configuration

```
SD Card Pin Configuration (Heltec Wireless Tracker):
┌─────────────────────────────────────┐
│ SD_CS      → Pin 4                  │
│ SD_MOSI    → Pin 41 (Shared SPI)    │
│ SD_SCLK    → Pin 40 (Shared SPI)    │
│ SD_MISO    → Pin 12 (Shared SPI)    │
└─────────────────────────────────────┘

Firmware Configuration:
#define SD_CS  4   // Chip Select SD Card
```

> **Catatan:** Pin SD Card CS menggunakan GPIO 4. Pastikan tidak conflict dengan pin lain (LoRa CS = GPIO 8).

---

## 3. TFT Display 3 Pages Scrolling

### 3.1 Cara Kerja Sistem

Sistem TFT menggunakan **FreeRTOS task** untuk menampilkan 3 halaman yang bergantian secara otomatis:

```
┌─────────────────────────┐
│     PAGE 1: LoRaWAN      │  ← Setiap 3 detik berganti
├─────────────────────────┤
│     PAGE 2: GPS         │
├─────────────────────────┤
│     PAGE 3: IR Status   │
└─────────────────────────┘
```

**Konfigurasi Utama** (di file firmware):

```cpp
#define TFT_MAX_PAGES     3       // Jumlah halaman
#define PAGE_SWITCH_MS    3000   // Waktu tampil per halaman (3 detik)
```

### 3.2 Struktur Data Halaman

```cpp
struct TftPageData {
    String rows[TFT_ROWS_PER_PAGE];  // 6 baris per halaman
};

// Inisialisasi awal
TftPageData g_TftPageData[TFT_MAX_PAGES] = {
    {"LoRaWAN Status", "-", "-", "-", "-", "-"},           // Page 0: LoRaWAN
    {"GPS Status", "Lat: -", "Long: -", "Alt: -", "Sat: -", "Time: --:--:--"},  // Page 1: GPS
    {"IR Receiver", "Waiting...", "-", "-", "-", "-"}       // Page 2: IR
};
```

### 3.3 Isi Masing-masing Halaman

#### **Halaman 0: LoRaWAN Status**

```
┌─────────────────────────────────────┐
│ LoRaWAN Status                      │
│ Event: TX_OK                        │
│ RSSI: -75 dBm                       │
│ SNR: 7.5 dB                         │
│ Batt: 4123 mV                       │
│ Joined: YES                         │
└─────────────────────────────────────┘
```

Update di fungsi `loraTask()`:

```cpp
void loraTask(void *pv) {
    // ... kode lain ...
    
    // Update TFT LoRa page
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[0].rows[0] = "LoRaWAN Status";
        g_TftPageData[0].rows[1] = "Event: " + eventStr;
        g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
        g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
        g_TftPageData[0].rows[4] = "Batt: " + String(battery_mV) + " mV";
        g_TftPageData[0].rows[5] = g_loraStatus.joined ? "Joined: YES" : "Joined: NO";
        xSemaphoreGive(xTftMutex);
    }
}
```

#### **Halaman 1: GPS Data**

```
┌─────────────────────────────────────┐
│ GPS Status                          │
│ Lat: -6.208800                      │
│ Long: 106.845600                    │
│ Alt: 45.5 m                        │
│ Sat: 8                              │
│ Time: 10:30:45 (WIB)               │
└─────────────────────────────────────┘
```

Update di fungsi `gpsTask()`:

```cpp
void gpsTask(void *pv) {
    // ... kode GPS processing ...
    
    // Update TFT GPS page
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[1].rows[0] = "GPS Status";
        g_TftPageData[1].rows[1] = hasLoc ? "Lat: " + String(g_gpsData.lat, 6) : "Lat: -";
        g_TftPageData[1].rows[2] = hasLoc ? "Long: " + String(g_gpsData.lng, 6) : "Long: -";
        g_TftPageData[1].rows[3] = hasLoc ? "Alt: " + String(g_gpsData.alt, 1) + " m" : "Alt: -";
        g_TftPageData[1].rows[4] = "Sat: " + String(g_gpsData.satellites);
        g_TftPageData[1].rows[5] = "Time: " + String(timeBuf);
        xSemaphoreGive(xTftMutex);
    }
}
```

#### **Halaman 2: IR Status**

```
┌─────────────────────────────────────┐
│ IR Receiver                         │
│ Proto: NEC                          │
│ Addr: 0xABCD                        │
│ Cmd: 0x12                           │
│ Last: 5s ago                        │
│ Status: HIT DETECTED!               │
└─────────────────────────────────────┘
```

Update di fungsi `irTask()`:

```cpp
void irTask(void *pv) {
    // ... kode IR processing ...
    
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[2].rows[0] = "IR Receiver";
        if (g_irStatus.dataReceived) {
            g_TftPageData[2].rows[1] = "Proto: " + g_irStatus.protocol;
            g_TftPageData[2].rows[2] = "Addr: 0x" + String(g_irStatus.address, HEX);
            g_TftPageData[2].rows[3] = "Cmd: 0x" + String(g_irStatus.command, HEX);
            g_TftPageData[2].rows[4] = "Last: " + String(ago) + "s ago";
            g_TftPageData[2].rows[5] = "Status: HIT!";
        } else {
            g_TftPageData[2].rows[1] = "Waiting...";
            g_TftPageData[2].rows[2] = "-";
            g_TftPageData[2].rows[3] = "-";
            g_TftPageData[2].rows[4] = "-";
            g_TftPageData[2].rows[5] = "-";
        }
        xSemaphoreGive(xTftMutex);
    }
}
```

### 3.4 Cara Customize Halaman

#### **Contoh 1: Ubah Halaman 1 jadi tampilkan Speed**

Cari di `gpsTask()`:

```cpp
// SEBELUM:
g_TftPageData[1].rows[3] = hasLoc ? "Alt: " + String(g_gpsData.alt, 1) + " m" : "Alt: -";

// SESUDAH (tampilkan speed):
g_TftPageData[1].rows[3] = hasLoc ? "Speed: " + String(GPS.speed.knots(), 1) + " kn" : "Speed: -";
```

#### **Contoh 2: Tambah Device ID di Halaman 0**

```cpp
// Di loraTask(), update:
g_TftPageData[0].rows[0] = "Device: " + String(DEVICE_ID);
```

#### **Contoh 3: Ubah Format Battery**

```cpp
// SEBELUM:
g_TftPageData[0].rows[4] = "Batt: " + String(battery_mV) + " mV";

// SESUDAH (tampilkan percentage):
int batteryPct = map(battery_mV, 3000, 4200, 0, 100);
batteryPct = constrain(batteryPct, 0, 100);
g_TftPageData[0].rows[4] = "Batt: " + String(batteryPct) + "%";
```

### 3.5 Tambah Halaman Baru (4 Halaman)

#### **Langkah 1: Ubah konstanta**

```cpp
#define TFT_MAX_PAGES     4   // Ubah dari 3 ke 4
```

#### **Langkah 2: Tambah inisialisasi halaman**

```cpp
TftPageData g_TftPageData[TFT_MAX_PAGES] = {
    {"LoRaWAN Status", "-", "-", "-", "-", "-"},           // Page 0: LoRaWAN
    {"GPS Status", "Lat: -", "Long: -", "Alt: -", "Sat: -", "Time: --:--:--"},  // Page 1: GPS
    {"IR Receiver", "Waiting...", "-", "-", "-", "-"},       // Page 2: IR
    {"SYSTEM", "Ver: 1.1", "Uptime: -", "FreeMem: -", "IP: -", "-"}  // Page 3: System Info (BARU)
};
```

#### **Langkah 3: Update tftTask() untuk halaman baru**

```cpp
void tftTask(void *pv) {
    uint32_t pageSwitch = millis();
    uint8_t currentPage = 0;

    for (;;) {
        if (millis() - pageSwitch >= PAGE_SWITCH_MS) {
            currentPage = (currentPage + 1) % TFT_MAX_PAGES;  // Sekarang sampai 3
            pageSwitch = millis();
        }
        // ... rest of code ...
    }
}
```

#### **Langkah 4: Tambah task untuk update halaman system info**

```cpp
void systemInfoTask(void *pv) {
    for (;;) {
        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            uint32_t freeMem = esp_get_free_heap_size();
            uint32_t uptimeSec = millis() / 1000;
            int hours = uptimeSec / 3600;
            int mins = (uptimeSec % 3600) / 60;
            
            g_TftPageData[3].rows[0] = "SYSTEM INFO";
            g_TftPageData[3].rows[1] = "Ver: 1.1.0";
            g_TftPageData[3].rows[2] = "Uptime: " + String(hours) + "h " + String(mins) + "m";
            g_TftPageData[3].rows[3] = "FreeMem: " + String(freeMem) + " B";
            g_TftPageData[3].rows[4] = g_wifiStatus.connected ? "IP: " + g_wifiStatus.ip : "WiFi: Disconnected";
            g_TftPageData[3].rows[5] = "-";
            xSemaphoreGive(xTftMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

#### **Langkah 5: Register task di setup()**

```cpp
void setup() {
    // ... existing code ...
    
    xTaskCreatePinnedToCore(systemInfoTask, "SysInfo", 2048, NULL, 1, NULL, 0);
}
```

### 3.6 Ubah Waktu Tiap Halaman

```cpp
#define PAGE_SWITCH_MS 3000   // 3 detik (default)

// Ubah sesuai kebutuhan:
#define PAGE_SWITCH_MS 5000   // 5 detik
#define PAGE_SWITCH_MS 2000   // 2 detik
```

### 3.7 TFT Pin Configuration

```
┌─────────────────────────────────────────┐
│ TFT Pin Mapping (Heltec Wireless Track) │
├─────────────────────────────────────────┤
│ TFT_CS    → Pin 38                      │
│ TFT_DC    → Pin 40                      │
│ TFT_RST   → Pin 39                      │
│ TFT_SCLK  → Pin 41                      │
│ TFT_MOSI  → Pin 42                      │
│ TFT LED   → Pin 21 (LED_K)              │
└─────────────────────────────────────────┘
```

### 3.8 Troubleshooting TFT

| Masalah | Solusi |
|---------|--------|
| TFT tidak tampil | Cek Vext_Ctrl Pin 3 = HIGH (power on) |
| Tampilan corrupt | Tambah delay sebelum update display |
| Scroll terlalu cepat | Tambah `PAGE_SWITCH_MS` |
| Huruf tidak terbaca | Cek `framebuffer.setTextSize()` |

---

## Quick Reference

### URLs Penting

```
Dashboard:       https://mygps-tracker.vercel.app/dashboard
API Track:       https://mygps-tracker.vercel.app/api/track
API TTN:         https://mygps-tracker.vercel.app/api/ttn
Neon Console:    https://console.neon.tech
TTN Console:     https://console.thethingsnetwork.org
Vercel Dashboard: https://vercel.com/dashboard
```

### Data Flow

```
┌──────────────┐     WiFi      ┌─────────────────┐
│   ESP32      │ ────────────→ │  /api/track     │
│  Firmware    │               └────────┬────────┘
└──────────────┘                        │
                                        ▼
┌──────────────┐     LoRaWAN    ┌─────────────────┐
│   ESP32      │ ────────────→ │  /api/ttn       │ ──┐
│  (TTN)       │               └─────────────────┘   │
└──────────────┘                                      │
                                      ┌──────────────┴──────────────┐
                                      ▼                              ▼
                              ┌─────────────┐               ┌─────────────┐
                              │  NeonDB     │ ←──────────→ │  Vercel     │
                              │  (PostgreSQL)│               │  Dashboard  │
                              └─────────────┘               └─────────────┘
```

---

## Credits

Project ini dibuat untuk **thesis/seminar hasil** laser tag system dengan:
- **Hardware**: ESP32-S3, Heltec Wireless Tracker, LoRa SX1262, GPS, IR Receiver
- **Firmware**: Arduino C++ dengan FreeRTOS
- **Web**: Next.js 16 + Prisma + PostgreSQL
- **Network**: WiFi HTTP + LoRaWAN (TTN)

---

## APPENDIX A: Info Vercel Lengkap

### A.1 Cara Daftar Vercel

#### Langkah-langkah:

1. **Buka website Vercel**
   - Buka browser (Chrome/Edge/Firefox/Safari)
   - Kunjungi: **https://vercel.com**

2. **Klik "Sign Up"**
   - Di pojok kanan atas, klik **"Sign Up"**

3. **Pilih metode login**
   - **Opsi 1 (Recomend): Klik "Continue with GitHub"**
     - Akan membuka halaman GitHub
     - Login dengan akun GitHub Anda
     - Klik "Authorize vercel"
   - **Opsi 2: Continue with Email**
     - Masukkan email Anda
     - Klik "Continue"
     - Buka email untuk verifikasi

4. **Isi informasi tambahan**
   - **Name**: Nama lengkap Anda
   - **Team Name**: (opsional) bisa isi atau skip
   - Klik "Continue"

5. **Selesai!**
   - Anda akan masuk ke dashboard Vercel
   - Klik "Add New" → "Project" untuk mulai deploy

---

### A.2 Cara Deploy Project ke Vercel

#### Cara 1: Dari GitHub (Recomend)

1. **Login ke Vercel** dengan GitHub
2. Klik **"Add New..."** → **"Project"**
3. **Cari repository**
   - Di "Import Git Repository", ketik: `latifannurdiansyah/Laser-tag-project`
   - Klik **"Import"**
4. **Setup Environment Variables**
   - Bagian "Environment Variables", klik **"Add"**
   - Isi:
     - **Name**: `DATABASE_URL`
     - **Value**: `postgresql://neondb_owner:PASSWORD@ep-xyz.us-east-1.aws.neon.tech/neondb?sslmode=require`
       - Ganti `PASSWORD` dengan password NeonDB Anda
   - Klik **"Save"**
5. **Deploy**
   - Klik tombol **"Deploy"** (biru)
   - Tunggu 1-3 menit
   - Jika berhasil, akan muncul ✅ hijau

#### Cara 2: Lewat Vercel CLI

```bash
# Install Vercel CLI
npm i -g vercel

# Login
vercel login

# Masuk ke folder project
cd my-gps-tracker

# Deploy
vercel --prod
```

---

### A.3 Cara Menjadikan Project Public

**Default: Vercel project adalah PUBLIC**

Artinya:
- ✅ Semua orang bisa akses link Anda tanpa login
- ✅ Tidak perlu autentikasi untuk lihat dashboard
- ✅ Cocok untuk project laser tag (player bisa lihat posisi)

#### Jika Ingin Private (Semua Orang Harus Login):

1. Buka **Vercel Dashboard**
2. Klik project Anda
3. Klik **"Settings"**
4. Scroll ke **"Git"**
5. Matikan toggle **"Make project public"**

#### Cara Menggunakan Custom Domain (Optional):

1. Buka **Settings** → **Domains**
2. Masukkan domain Anda (misal: `gps-laser.tag`)
3. Ikuti instruksi DNS yang diberikan Vercel
4. Tunggu propagate (maksimal 24 jam)

---

### A.4 Vercel Free Tier (Hobby) - Apa Gratis dan Sampai Kapan?

#### Vercel Hobby (Gratis)

| Aspek | Detail |
|-------|--------|
| **Harga** | **GRATIS** |
| **Berlaku** | **Selamanya** (selama tidak违反 ToS) |
| **Bandwidth** | 100 GB/bulan |
| **Build Minutes** | 100 menit/bulan |
| **Deployment** | Unlimited |
| **Custom Domain** | ✅ Gratis dengan SSL |
| **Team Members** | 1 orang (anda) |
| **Serverless Functions** | ✅ Ada |
| **Edge Functions** | ✅ Ada |

#### Kapan Mulai Bayar? (Upgrade ke Pro)

| Kondisi | Solusi |
|---------|--------|
| Traffic > 100 GB/bulan | Upgrade ke Pro ($20/bulan) |
| Build > 100 menit/bulan | Upgrade ke Pro |
| Butuh team > 1 orang | Upgrade ke Pro |
| Butuh analisis lanjutan | Upgrade ke Pro |

#### Tips Agar Tetap Gratis:

1. **Optimasi gambar** - Gunakan format WebP
2. **Cache yang baik** - Manfaatkan Next.js Image optimization
3. **Hapus deployment yang tidak perlu** - Buka Deployments, hapus yang lama
4. **Gunakan lazy loading** - Hanya load data yang diperlukan

---

### A.5 Penyimpanan dan Limit Database

#### NeonDB (PostgreSQL)

| Aspek | Limit Free Tier |
|-------|-----------------|
| **Penyimpanan** | 0.5 GB (512 MB) |
| **Connections** | Max 100 koneksi |
| **Region** | Singapore (terdekat) |
| **Backup** | Otomatis |

#### Tips Hemat Penyimpanan:

1. **Hapus data lama secara berkala** - Buat script untuk hapus data > 30 hari
2. **Gunakan proper indexing** - Agar query lebih cepat
3. **Archive data lama** - Export ke JSON sebelum hapus

---

### A.6 Credentials yang Dipakai

#### 1. GitHub
- **Username**: Akun GitHub Anda
- **Password**: Password GitHub (atau SSH key)
- **Login via**: OAuth (klik "Continue with GitHub")

#### 2. Vercel
- **Login**: Via GitHub (OAuth)
- **Project Settings**: Di dashboard Vercel
- **Environment Variables**: `DATABASE_URL`

#### 3. NeonDB (Database)
- **Connection String**: `postgresql://user:password@host/dbname?sslmode=require`
- **User**: `neondb_owner` (default)
- **Password**: Password yang Anda buat saat buat project Neon
- **Host**: `ep-xxx.us-east-1.aws.neon.tech`
- **Database Name**: `neondb`

#### 4. TTN (The Things Network)
- **DevEUI**: 8 bytes hex (device identifier)
- **JoinEUI/AppEUI**: 8 bytes hex (application identifier)
- **AppKey**: 16 bytes hex (encryption key)
- **NwkKey**: 16 bytes hex (network key)

#### 5. WiFi (Firmware)
- **SSID**: Nama WiFi Anda
- **Password**: Password WiFi

---

### A.7 Troubleshooting Vercel

| Masalah | Solusi |
|---------|--------|
| Deploy gagal | Cek Build Logs di Vercel |
| Database connection error | Pastikan DATABASE_URL benar |
| 500 Error saat akses | Cek Vercel Function logs |
| SSL error (custom domain) | Tunggu propagate atau cek DNS |
| Build timeout | Perbesar plan atau optimasi build |

#### Cara Melihat Logs:

1. Buka **Vercel Dashboard**
2. Klik project Anda
3. Klik **"Deployments"**
4. Klik deployment terbaru
5. Scroll ke bawah untuk lihat **"Function Logs"**

---

## APPENDIX B: Ringkasan Semua Akun & Credentials

| Service | Cara Daftar | Credentials | URL Login |
|---------|-------------|-------------|-----------|
| **GitHub** | github.com → Sign Up | Username + Password | github.com |
| **Vercel** | vercel.com → Sign Up with GitHub | Login via GitHub | vercel.com |
| **NeonDB** | console.neon.tech → Create Project | Email + Password | console.neon.tech |
| **TTN** | console.thethings.network → Login with GitHub | Login via GitHub | console.thethings.network |
| **Arduino IDE** | download dari arduino.cc | - | - |

---

## APPENDIX C: Links Penting

### Links Production

| Service | URL |
|---------|-----|
| **Dashboard** | `https://mygps-tracker.vercel.app/dashboard` |
| **API Track** | `https://mygps-tracker.vercel.app/api/track` |
| **API TTN** | `https://mygps-tracker.vercel.app/api/ttn` |
| **Random Test** | `https://mygps-tracker.vercel.app/api/random` |

### Links Console/Development

| Service | URL |
|---------|-----|
| **Vercel Dashboard** | https://vercel.com/dashboard |
| **NeonDB Console** | https://console.neon.tech |
| **TTN Console** | https://console.thethings.network |
| **GitHub Repo** | https://github.com/latifannurdiansyah/Laser-tag-project |

### Links Download

| Software | URL |
|----------|-----|
| **Arduino IDE** | https://www.arduino.cc/en/software |
| **Git for Windows** | https://git-scm.com/download/win |
| **Node.js** | https://nodejs.org/ |

---

**Catatan**: Simpan semua credentials di tempat aman! Untuk production, gunakan environment variables, jangan hardcode di kode.
