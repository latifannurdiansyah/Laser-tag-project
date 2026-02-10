# Laser Tag Project

A comprehensive laser tag game system with real-time GPS tracking, anti-cheating mechanisms, and cloud-based monitoring. This project integrates three main devices (weapons, helmets, and vests) to detect shots, process data, and send game results to a server in real-time.

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Firmware](#firmware)
- [Software Dashboard](#software-dashboard)
- [Communication Protocols](#communication-protocols)
- [Firebase Configuration](#firebase-configuration)
- [Setup Instructions](#setup-instructions)
- [Project Structure](#project-structure)
- [Features](#features)

---

## Overview

The system begins by activating all devices. Once the system is ready, players can shoot by pressing the push button on the gun. When the button is pressed, the infrared (IR) transmitter on the gun emits an infrared signal containing the shooter's ID data. This signal is directed at the opponent and can be received by the opponent's helmet or vest.

### How It Works

1. **Shooting**: Player presses the trigger button → IR transmitter sends signal with shooter ID
2. **Detection**: Helmet/Vest receives IR signal and verifies NEC protocol compliance
3. **Relay**: Helmet forwards data to Vest via ESP-NOW wireless communication
4. **Processing**: Vest adds GPS location data and timestamp to the hit information
5. **Transmission**: Vest sends complete data packet via LoRa to the Gateway
6. **Cloud Upload**: Gateway uploads data to Firebase Realtime Database
7. **Monitoring**: Web dashboard displays real-time game data

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    LASER TAG SYSTEM ARCHITECTURE                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────┐    IR (NEC Protocol)    ┌──────────┐           │
│  │ WEAPON   │ ───────────────────────► │  HELMET  │           │
│  │ (Gun)    │                         │          │           │
│  └──────────┘                         └────┬─────┘           │
│       │                                      │                 │
│       │                                      │ ESP-NOW         │
│       ▼                                      ▼                 │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                     VEST (Rompi)                      │      │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────────────┐    │      │
│  │  │ IR Recv │  │ Process  │  │ Add GPS + Time   │    │      │
│  │  └─────────┘  └──────────┘  └────────┬─────────┘    │      │
│  └───────────────────────────────────────┼───────────────┘      │
│                                        │                       │
│                                        │ LoRa (SX1262)         │
│                                        ▼                       │
│  ┌──────────────────────────────────────────────────────┐      │
│  │              LORA GATEWAY / GPS TRACKER               │      │
│  │              Heltec Wireless Tracker V1.2               │      │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────────────┐    │      │
│  │  │ GPS     │  │ LoRa Rx  │  │ WiFi + Firebase │    │      │
│  │  │ Module  │  │          │  │                  │    │      │
│  │  └─────────┘  └──────────┘  └──────────────────┘    │      │
│  └──────────────────────────────────────────────────────┘      │
│                                        │                       │
│                                        │ WiFi                  │
│                                        ▼                       │
│                              ┌──────────────────┐              │
│                              │  FIREBASE RTDB   │              │
│                              │  (Cloud Server)  │              │
│                              └────────┬─────────┘              │
│                                       │                        │
│                                       ▼                        │
│                              ┌──────────────────┐              │
│                              │  Web Dashboard   │              │
│                              │  (src/index.html)│              │
│                              └──────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Hardware Components

### 1. Weapon (Senjata)
- **Microcontroller**: ESP32
- **Components**:
  - IR Transmitter LED (38kHz)
  - Push button (trigger)
  - Power management circuit

### 2. Helmet (Helm)
- **Board**: ESP32-S3 Supermini
- **Components**:
  - IR Receiver (VS1838B or similar)
  - IR Transmitter LED
  - Photodiode (anti-cheating sensor)
  - Status LEDs

### 3. Vest (Rompi)
- **Board**: ESP32-based custom board
- **Components**:
  - IR Receiver
  - ESP-NOW communication module
  - LoRa module (SX1262)
  - GPS module
  - Vibration motor (hit feedback)

### 4. Gateway / GPS Tracker
- **Board**: Heltec Wireless Tracker V1.2 (ESP32-S3 based)
- **Components**:
  - GPS module (NEO-6M or built-in)
  - LoRa module (SX1262)
  - TFT Display (160x80, ST7735)
  - WiFi module
  - Battery management

---

## Firmware

### Main Firmware Location
`firmware/LaserTagProjectFirmwareV1/`

### Key Firmware Files

| File/Directory | Purpose |
|----------------|---------|
| `LaserTagProjectFirmwareV1.ino` | Main firmware for GPS Tracker with Firebase |
| `firmware/Backup/ESP32S3 Supermini/Helm_IR_Anticheating_espnow/` | Helmet firmware with ESP-NOW and anti-cheating |
| `firmware/Backup/Heltec Wireless Tracker V1.2/heltec-tracker-espnow-lora-gps/` | Complete GPS + LoRa + ESP-NOW firmware |

### Helmet Firmware Components

```cpp
// Helm_IR_Anticheating_espnow.ino
├── IRHandler.h         // NEC protocol IR handling
├── AntiCheat.h         // Anti-cheating mechanism
└── ESPNowHandler.h     // ESP-NOW wireless communication
```

### Gateway Firmware Modules

```
heltec-tracker-espnow-lora-gps/src/
├── PinConfig.h         // Pin definitions and configuration
├── ESPNowModule.ino    // ESP-NOW communication
├── GPSModule.ino       // GPS data handling (TinyGPS++)
├── LoRaModule.ino      // LoRa SX1262 transmission
├── TFTDisplay.ino      // TFT display rendering
└── main.ino            // Main application loop
```

### Libraries Required

- **Arduino.h** - Core Arduino framework
- **IRremote.hpp** - IR NEC protocol handling
- **WiFi.h / esp_now.h** - ESP-NOW wireless communication
- **TinyGPS++.h** - GPS data parsing
- **Adafruit_ST7735 / Adafruit_GFX** - TFT display
- **RadioLib** - LoRa SX1262 communication
- **Firebase_ESP_Client** - Firebase integration

---

## Software Dashboard

### Location
`src/` directory

### Files

| File | Description |
|------|-------------|
| `index.html` | Modern web dashboard with dark theme |
| `index.js` | Firebase integration and DevTools |
| `firebase.js` | Firebase SDK configuration |
| `package.json` | Node.js dependencies |

### Dashboard Features

- **Real-time GPS Tracking Table** - View all device locations
- **Firebase Structure Viewer** - Explore database structure
- **DevTools Panel**:
  - Set random coordinates for testing
  - Simulate GPS updates
  - Export/Import JSON data
  - Manual data editing
  - Clear all data

### Running the Dashboard

```bash
# Using Python (recommended)
cd src
python -m http.server 8080

# Then open browser
# http://localhost:8080/
```

---

## Communication Protocols

| Segment | Protocol | Frequency | Purpose |
|---------|----------|-----------|---------|
| Weapon → Target | IR NEC | 38kHz IR LED | Shoot detection |
| Helmet → Vest | ESP-NOW | 2.4GHz WiFi | Short-range wireless |
| Vest → Gateway | LoRa (SX1262) | 923MHz (Asia) | Long-range transmission |
| Gateway → Cloud | WiFi | 2.4GHz | Internet connection |

### NEC Protocol Format

```
┌─────────────────────────────────────────────┐
│  Address  │ Command │ ~Command │   Gap    │
│  8 bits   │ 8 bits  │  8 bits  │  40ms+   │
└─────────────────────────────────────────────┘
```

### ESP-NOW Data Structure

```cpp
typedef struct {
  uint16_t address;  // Shooter ID (NEC address)
  uint8_t  command;  // Shot command (NEC command)
} ir_data_t;
```

### LoRa Payload Structure

```cpp
typedef struct __attribute__((packed)) {
  uint8_t  header;      // 0xAA
  uint8_t  groupId;     // Game group ID
  uint8_t  nodeId;      // Node identifier
  int32_t  lat;         // Latitude * 1000000
  int32_t  lng;         // Longitude * 1000000
  int16_t  alt;         // Altitude (meters)
  uint8_t  satellites;  // GPS satellites count
  uint16_t battery;     // Battery voltage
  uint32_t uptime;      // Uptime in seconds
  uint16_t irAddress;   // Shooter address
  uint8_t  irCommand;   // Shot command
  uint8_t  hitStatus;   // Hit detected flag
  uint8_t  checksum;    // XOR checksum
} lora_payload_t;
```

### LoRa Configuration

```cpp
#define LORA_FREQUENCY       923.0      // MHz (Asia)
#define LORA_BANDWIDTH       125.0      // kHz
#define LORA_SPREADING_FACTOR 10        // SF10
#define LORA_CODING_RATE     5
#define LORA_TX_POWER        22         // dBm
#define LORA_SYNC_WORD       0x12
```

---

## Firebase Configuration

### Project Details

- **Project ID**: `gps-log-a1d90`
- **Database URL**: `https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app`

### Firebase SDK Configuration

```javascript
const firebaseConfig = {
  apiKey: "AIzaSyBknkRTKdVwZInvip3SrzWDIdpefDRoIWI",
  authDomain: "gps-log-a1d90.firebaseapp.com",
  databaseURL: "https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "gps-log-a1d90",
  storageBucket: "gps-log-a1d90.firebasestorage.app",
  messagingSenderId: "670421186715",
  appId: "1:670421186715:web:d01f0c518b09f300de2e3e"
};
```

### Data Structure

```
gps_tracking/
├── latitude    (float)
├── longitude   (float)
├── altitude    (integer)
└── satellites  (integer)
```

### Firebase Rules (for development)

```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

---

## Setup Instructions

### Hardware Setup

1. **Heltec Wireless Tracker V1.2**
   - Connect USB-C for power/programming
   - GPS module is built-in
   - LoRa module is built-in (SX1262)
   - TFT display is built-in

2. **ESP32S3 Supermini (Helmet)**
   - IR receiver on pin 4
   - IR LED on pin 6
   - Photodiode on pin 5
   - Configure peer MAC address for ESP-NOW

3. **Vest Board**
   - IR receiver for hit detection
   - ESP-NOW module for helmet communication
   - LoRa module for gateway transmission
   - GPS module for location tracking

### Firmware Upload

1. **For Heltec Tracker**:
   - Open Arduino IDE
   - Select board: "Heltec Wireless Tracker V1.2"
   - Select USB port
   - Upload `firmware/LaserTagProjectFirmwareV1/LaserTagProjectFirmwareV1.ino`

2. **For ESP32S3 Supermini (Helmet)**:
   - Select board: "ESP32S3 Dev Module"
   - Configure flash size and partition
   - Upload firmware from `firmware/Backup/ESP32S3 Supermini/`

### WiFi Configuration

Edit the following in firmware files:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

### Firebase Configuration

Edit authentication token in firmware:

```cpp
#define FIREBASE_HOST "gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "YOUR_FIREBASE_AUTH_TOKEN"
```

---

## Project Structure

```
Laser-tag-project/
├── README.md                    # This file
├── .gitignore                   # Git ignore rules
│
├── firmware/                    # All firmware files
│   ├── LaserTagProjectFirmwareV1/
│   │   └── LaserTagProjectFirmwareV1.ino
│   └── Backup/
│       ├── ESP32S3 Supermini/
│       │   ├── HELM_ESPNOW_IR/
│       │   ├── Helm_IR_Anticheating_espnow/
│       │   │   ├── Anticheat.h
│       │   │   ├── ESPNowHandler.h
│       │   │   ├── Helm_IR_Anticheating_espnow.ino
│       │   │   └── IRHandler.h
│       │   ├── Helm_IR_Anticheating_espnow_jadisatu/
│       │   ├── Helm_IR_anticheating/
│       │   ├── SAT_Receiver/
│       │   └── SAT_Transmitter_FREERTOS/
│       └── Heltec Wireless Tracker V1.2/
│           ├── heltec-tracker-espnow-lora-gps/
│           │   ├── platformio.ini
│           │   ├── src/
│           │   │   ├── PinConfig.h
│           │   │   ├── ESPNowModule.ino
│           │   │   ├── GPSModule.ino
│           │   │   ├── LoRaModule.ino
│           │   │   ├── TFTDisplay.ino
│           │   │   └── main.ino
│           │   └── include/
│           ├── GPS_TFT_Wifi_firebase/
│           └── heltec-tracker-gateway/
│
├── src/                        # Web dashboard
│   ├── index.html             # Main dashboard UI
│   ├── index.js               # Dashboard logic & Firebase
│   ├── firebase.js            # Firebase SDK config
│   ├── package.json           # Node.js dependencies
│   └── node_modules/           # Installed packages
│       └── firebase/          # Firebase SDK
│
└── docs/                       # Documentation
    ├── Ujian Semhas/          # Thesis defense documents
    │   ├── F-01 Checklist JTE.docx
    │   ├── F-07 Nilai Pembimbing I Proyek Akhir.docx
    │   ├── F-07 Nilai Pembimbing II Proyek Akhir.docx
    │   ├── F-08 Izin Maju Ujian oleh Pembimbing.docx
    │   ├── F-09 Berita Acara Ujian Skripsi.docx
    │   ├── F-10 Nilai Penguji Proyek Akhir.docx
    │   ├── F-11 REVISI SkripsiLA JTE.docx
    │   ├── F-12 Penyerahan Alat JTE.docx
    │   ├── HALAMAN PENGESAHAN.docx
    │   ├── Pedoman Petunjuk Operasional (Manual Book).docx
    │   └── README.md
    ├── final-report.docx
    └── templateJASENS.docx
```

---

## Features

### Core Features

- ✅ **IR Shooting System**: NEC protocol-based shot detection
- ✅ **Anti-Cheating**: Photodiode verification prevents fake signals
- ✅ **GPS Tracking**: Real-time location monitoring
- ✅ **Multi-hop Communication**: IR → ESP-NOW → LoRa → WiFi → Cloud
- ✅ **Real-time Dashboard**: Live data visualization
- ✅ **Data Logging**: Firebase Realtime Database storage

### Anti-Cheating Mechanism

The system implements a dual-verification approach:

1. **NEC Protocol Validation**: Only valid IR signals following NEC format are accepted
2. **Photodiode Verification**: Signal must be accompanied by a valid light pulse detected by photodiode

This prevents players from using external IR sources or DIY transmitters.

### Dashboard Capabilities

- **Live Monitoring**: Real-time table view of all devices
- **Data Visualization**: Color-coded status indicators
- **DevTools**: Built-in testing and debugging tools
- **Export/Import**: JSON data management
- **Responsive Design**: Works on desktop and mobile

### Communication Range

| Technology | Typical Range | Notes |
|------------|--------------|-------|
| IR (38kHz) | 3-5 meters | Line of sight required |
| ESP-NOW | 50-100 meters | 2.4GHz WiFi based |
| LoRa (923MHz) | 1-2 km | Depends on environment |
| WiFi | 50-100 meters | Infrastructure dependent |

---

## Troubleshooting

### Common Issues

1. **GPS Fix Not Obtained**
   - Ensure outdoor location with clear sky view
   - Wait 2-5 minutes for cold start
   - Check antenna connection

2. **Firebase Connection Failed**
   - Verify Firebase rules allow read/write
   - Check internet connection
   - Validate authentication token

3. **ESP-NOW Not Working**
   - Verify both devices on same WiFi channel
   - Check MAC address configuration
   - Ensure peer device is powered on

4. **LoRa Transmission Failed**
   - Check antenna connection
   - Verify frequency settings (923MHz for Asia)
   - Check transmission power limits

### Debug Output

Enable serial debugging at 115200 baud rate:

```
Serial.begin(115200);
```

---

## License

This project is developed as a thesis project. All rights reserved.

---

## Author

Developed as a final year project (Proyek Akhir) for JTE (Jurusan Teknik Elektro).

---

## Acknowledgments

- Arduino/ESP32 Community
- Firebase Documentation
- RadioLib Library
- TinyGPS++ Library
- Adafruit Libraries

---

## 📚 Setup Guide (Indonesia)

Untuk panduan lengkap setup dalam bahasa Indonesia:

- **[docs/SETUP-GUIDE.md](docs/SETUP-GUIDE.md)** - Panduan lengkap NeonDB, TTN, dan TFT Display

### Quick Links dari Setup Guide:

| Topic | Section |
|-------|---------|
| NeonDB → Vercel | [docs/SETUP-GUIDE.md#1-neondb--vercel-connection](docs/SETUP-GUIDE.md#1-neondb--vercel-connection) |
| TTN Integration | [docs/SETUP-GUIDE.md#2-ttn-integration](docs/SETUP-GUIDE.md#2-ttn-integration) |
| TFT Display | [docs/SETUP-GUIDE.md#3-tft-display-3-pages-scrolling](docs/SETUP-GUIDE.md#3-tft-display-3-pages-scrolling) |
| URLs Penting | [docs/SETUP-GUIDE.md#quick-reference](docs/SETUP-GUIDE.md#quick-reference) |
