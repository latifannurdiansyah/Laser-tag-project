# Laser Tag Project - Complete Setup Guide

## Daftar Isi

0. [Setup my-gps-tracker Project](#0-setup-my-gps-tracker-project)
1. [NeonDB → Vercel Connection](#1-neondb--vercel-connection)
2. [TTN Integration](#2-ttn-integration)
3. [TFT Display 3 Pages Scrolling](#3-tft-display-3-pages-scrolling)

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

1. Buka **https://console.thethingsnetwork.org**
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
DevEUI:     70B3D57ED00739E0
JoinEUI:    0000000000000011
AppKey:     DB51EB2C50138482E2FBD264E7876427
```

### 2.4 Configure Webhook ke Vercel

1. Di TTN Console, klik **"Integrations"** → **"Webhooks"**
2. Klik **"Add webhook"** → **"Custom webhook"**
3. Isi:
   ```
   Webhook ID:       vercel-gps-tracker
   Webhook URL:      https://laser-tag-project.vercel.app/api/ttn
   Method:           POST
   Uplink message:   ✅ Enabled (centang)
   ```
4. Klik **"Add Webhook"**

### 2.5 Setup Payload Formatter

1. Di TTN Console, klik **"Payload Formats"** → **"Uplink"**
2. Replace dengan kode ini:

```javascript
function decodeUplink(input) {
    var data = {};
    var bytes = input.bytes;
    
    function readFloat32LE(b, o) {
        var sign = (b[o + 3] & 0x80) !== 0 ? -1 : 1;
        var exp = ((b[o + 3] & 0x7F) << 1) | ((b[o + 2] & 0x80) !== 0 ? 1 : 0);
        var mant = ((b[o + 2] & 0x7F) << 16) | (b[o + 1] << 8) | b[o];
        if (exp === 0) return sign * mant * Math.pow(2, -126);
        if (exp === 255) return mant !== 0 ? NaN : sign * Infinity;
        return sign * (1 + mant / 8388608) * Math.pow(2, exp - 127);
    }
    
    data.lat = readFloat32LE(bytes, 11);
    data.lng = readFloat32LE(bytes, 15);
    data.alt = readFloat32LE(bytes, 19);
    
    data.address_id = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    data.sub_address_id = bytes[4];
    data.shooter_address_id = (bytes[8] << 24) | (bytes[7] << 16) | (bytes[6] << 8) | bytes[5];
    data.status = bytes[10];
    data.battery_mv = (bytes[24] << 8) | bytes[23];
    data.satellites = bytes[25];
    data.rssi = (bytes[27] << 8) | bytes[26];
    data.snr = bytes[28];
    
    // Device ID: Heltec-P1, Heltec-P2, Heltec-P3... (based on address_id)
    var playerNum = data.address_id + 1;
    data.deviceId = "Heltec-P" + playerNum.toString();
    
    data.irStatus = data.status === 1 
        ? "HIT: 0x" + data.shooter_address_id.toString(16).toUpperCase() + "-0x" + data.sub_address_id.toString(16).toUpperCase()
        : "-";
    
    return {
        data: {
            deviceId: data.deviceId,
            lat: data.lat,
            lng: data.lng,
            alt: data.alt,
            irStatus: data.irStatus,
            battery: data.battery_mv,
            satellites: data.satellites,
            rssi: data.rssi,
            snr: data.snr
        }
    };
}
```

3. Klik **"Save payload function"**

### Device ID Berdasarkan address_id

| address_id (di firmware) | Device ID (di dashboard) |
|---------------------------|---------------------------|
| `0x00000000` | Heltec-P1 |
| `0x00000001` | Heltec-P2 |
| `0x00000002` | Heltec-P3 |

### 2.6 Configure Firmware

1. Buka file:
   ```
   firmware/heltec-ir-gps-lora-playloadttn-vercel/heltec_tracker_all_in_one.ino
   ```

2. Cari bagian **LoRaWAN Credentials** (baris ~34):

```cpp
const uint64_t joinEUI = 0x0000000000000011;  // GANTI DENGAN JoinEUI Anda
const uint64_t devEUI = 0x70B3D57ED00739E0;   // GANTI DENGAN DevEUI Anda

const uint8_t appKey[] = {
    0xDB, 0x51, 0xEB, 0x2C, 0x50, 0x13, 0x84, 0x82,
    0xE2, 0xFB, 0xD2, 0x64, 0xE7, 0x87, 0x64, 0x27
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

4. Cek Vercel dashboard: `https://laser-tag-project.vercel.app/dashboard`
5. Data harus muncul dalam 30 detik (uplink interval)

### 2.8 Troubleshooting TTN

| Masalah | Solusi |
|---------|--------|
| Join never succeeds | Cek DevEUI/JoinEUI format (harus LSB/hex) |
| No uplink received | Cek frequency plan (AS923 untuk Indonesia) |
| Webhook tidak trigger | Cek TTN → Integration → Webhook → "Recent deliveries" |
| Payload decode error | Cek payload formatter, pastikan format biner benar |

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
Dashboard:       https://laser-tag-project.vercel.app/dashboard
API Track:       https://laser-tag-project.vercel.app/api/track
API TTN:         https://laser-tag-project.vercel.app/api/ttn
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
