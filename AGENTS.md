# AGENTS.md - Laser Tag Project Development Guide

This document provides guidelines for agents working on the Laser Tag Project codebase.

## Project Overview

This is a laser tag game system with:
- **Firmware**: Arduino/C++ for ESP32 microcontrollers
- **Web Dashboard**: Vanilla HTML/CSS/JS with Firebase integration
- **Hardware**: ESP32-S3, Heltec Wireless Tracker, IR modules, LoRa (SX1262), GPS

---

## Build Commands

### Web Dashboard (src/)

```bash
# Start development server
cd src
python -m http.server 8080

# Or using Node.js http-server
npx http-server -p 8080

# View at http://localhost:8080/
```

### Arduino Firmware (Arduino IDE Only)

**Steps to Upload Firmware:**
1. Open the `.ino` file in Arduino IDE
2. Select correct board:
   - `Heltec Wireless Tracker V1.2` for GPS tracker firmware
   - `ESP32S3 Dev Module` for helmet/vest firmware
3. Select the correct port (USB/UART)
4. Click Upload button (Ctrl+U)
5. Open Serial Monitor (Ctrl+Shift+M) to view debug output at 115200 baud

**Board Configuration:**
- **GPS Tracker Firmware**: `firmware/LaserTagProjectFirmwareV1/LaserTagProjectFirmwareV1.ino`
- **Firmware Configuration**: Copy `firmware/config.h.template` to `firmware/config.h` and fill in credentials

### Firmware Libraries Required

Install via Arduino Library Manager (Sketch → Include Library → Manage Libraries):

```
IRremote.hpp
TinyGPS++
Adafruit ST7735 and ST7789 Library
Adafruit GFX Library
RadioLib
Firebase Arduino ESP32 Client (by Mobizt)
ESP32 Arduino Core (installed via Board Manager)
```

---

## Testing

### Web Dashboard Testing

The web dashboard includes built-in DevTools (accessed via bottom panel):

```javascript
// DevTools functions available globally
DevTools.setRandomCoords()      // Set random GPS coordinates
DevTools.simulateGPSUpdate()    // Simulate GPS data update
DevTools.exportJSON()           // Export data to JSON file
DevTools.importJSON()           // Import data from JSON file
DevTools.clearAllData()         // Clear all Firebase data
DevTools.setData()              // Set GPS data manually
DevTools.updateData()           // Update specific path
DevTools.deleteData()           // Delete specific path
```

### Serial Debugging

Firmware outputs debug info at 115200 baud:

1. Open **Tools → Serial Monitor** in Arduino IDE
2. Set baud rate to **115200**
3. Or use a terminal program like PuTTY or TeraTerm

### GPS Testing

1. Test outdoors for clear satellite signal
2. Monitor `Serial.println()` output for NMEA data
3. Check "NO FIX" vs valid coordinates display
4. Verify Firebase data matches local display

---

## Code Style Guidelines

### Arduino/C++ Firmware (.ino, .h)

#### Imports
```cpp
// Standard Arduino headers first
#include <Arduino.h>
#include <SPI.h>

// External libraries
#include <Adafruit_ST7735.h>
#include <TinyGPS++.h>

// Project-specific headers
#include "PinConfig.h"
#include "ESPNowHandler.h"
```

#### Naming Conventions
```cpp
// Constants: UPPER_SNAKE_CASE
#define MAX_BUFFER_SIZE 256
#define LORA_FREQUENCY 923.0

// Global variables: lower_case with underscore
uint8_t senderMac[6];
float currentLat = 0.0;

// Functions: snake_case
void gpsFeed();
void loraSendData();
void tftInit();

// Classes: PascalCase
class ESPNowHandler { ... };
class IRHandler { ... };

// Structs: PascalCase with _t suffix
typedef struct {
    uint16_t address;
    uint8_t command;
} ir_data_t;
```

#### Pin Configuration
```cpp
// All pin definitions in header files
#define IR_RECEIVE_PIN   4
#define IR_LED_PIN       6
#define TFT_CS           38
#define GPS_RX_PIN       33
#define GPS_TX_PIN       34
```

#### Data Types
```cpp
// Use fixed-width types for protocol data
uint8_t   // 0-255 (bytes, commands)
uint16_t  // 0-65535 (addresses, IDs)
int32_t   // GPS coordinates * 1000000
float     // GPS display values, battery voltage
```

#### Protocol Structures (Packed)
```cpp
// Always pack structures for wireless transmission
typedef struct __attribute__((packed)) {
    uint8_t  header;
    uint8_t  groupId;
    int32_t  lat;         // lat * 1000000
    int32_t  lng;         // lng * 1000000
    uint16_t battery;
    uint8_t  checksum;
} lora_payload_t;
```

#### Error Handling
```cpp
// Check initialization results
bool loraInit() {
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed, code: "));
        Serial.println(state);
        return false;
    }
    return true;
}

// Handle Firebase errors
if (Firebase.RTDB.setJSON(&fbdo, "/path", &json)) {
    Serial.println("Success");
} else {
    Serial.println("Error: " + fbdo.errorReason());
}
```

#### Timing and Delays
```cpp
// Use millis() for non-blocking delays
static unsigned long lastUpdate = 0;
if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    // Do something every second
}

// Short delays for protocol timing
delay(10);   // ESP-NOW
delay(100);  // GPS settle
```

### HTML/CSS/JavaScript (src/)

#### HTML Structure
```html
<!-- Semantic HTML with proper accessibility -->
<main class="dashboard">
    <header class="header">
        <h1>Live Tracking GPS</h1>
    </header>
    <section class="card">
        <table>
            <thead>
                <tr><th>Header</th></tr>
            </thead>
            <tbody id="table-body"></tbody>
        </table>
    </section>
</main>
```

#### CSS Styling
```css
/* CSS custom properties */
:root {
    --bg-primary: #0f0f23;
    --accent-primary: #6366f1;
    --success: #22c55e;
}

/* BEM naming convention */
.card { }
.card__header { }
.card--highlighted { }

/* Responsive design */
@media (max-width: 768px) {
    .card { flex-direction: column; }
}
```

#### JavaScript (ES6 Modules)
```javascript
// Import Firebase SDK from CDN
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, onValue, set } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

// Constants in UPPER_SCASE
const GPS_UPDATE_INTERVAL = 1000;
const FIREBASE_PATH = '/gps_tracking';

// Functions: camelCase
function initDashboard() { }
function updateTable(data) { }
function showToast(message, type) { }

// Async functions for Firebase
async function sendToFirebase(path, data) {
    try {
        await set(ref(db, path), data);
        console.log('[SUCCESS] Data sent');
    } catch (error) {
        console.error('[ERROR]', error);
    }
}
```

#### Console Logging Format
```javascript
// Use consistent log format
console.log('[INFO] Message');
console.log('[SUCCESS] Operation completed');
console.log('[WARNING] Potential issue');
console.log('[ERROR] Error details:', error);
```

---

## Development Workflow

### Adding New Features

1. **Web Dashboard Changes:**
   - Edit `src/index.html` for UI
   - Edit `src/index.js` for logic
   - Test at `http://localhost:8080/`

2. **Firmware Changes:**
   - Edit `.ino` or `.h` files
   - Test with serial monitor
   - Verify on actual hardware

3. **Firebase Schema Changes:**
   - Update `src/firebase.js` config
   - Update firmware Firebase calls
   - Update dashboard display logic

### Communication Protocol Changes

When modifying protocols:

1. Update all relevant files:
   - `PinConfig.h` - struct definitions
   - `LoRaModule.ino` - packing/unpacking
   - `ESPNowHandler.h` - ESP-NOW structures

2. Maintain backwards compatibility or version field

3. Document changes in commit message

---

## Important File Locations

```
Laser-tag-project/
├── README.md                    # Project documentation
├── AGENTS.md                    # Development guide for agents
├── src/
│   ├── index.html             # Dashboard UI
│   ├── index.js               # Dashboard logic
│   └── firebase.js            # Firebase config
├── firmware/
│   ├── config.h.template      # Credentials template (copy to config.h)
│   ├── LaserTagProjectFirmwareV1/
│   │   └── LaserTagProjectFirmwareV1.ino
│   └── Backup/               # Backup/experimental firmware (future use)
└── docs/
    └── Ujian Semhas/               # Thesis documents
```

---

## Firebase Project Configuration

- **Project ID**: `gps-log-a1d90`
- **Database**: `https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app`
- **Data Path**: `/gps_tracking` (latitude, longitude, altitude, satellites)

---

## Notes for Agents

- This is a thesis/school project with both firmware and web components
- Hardware testing requires ESP32 development boards
- Firebase project is already configured - do not change credentials unless instructed
- Always test firmware changes with serial output enabled
- Web dashboard DevTools can simulate GPS data for testing without hardware
- **IMPORTANT**: Never commit `firmware/config.h` - it contains sensitive credentials
- Before compiling firmware, copy `firmware/config.h.template` to `firmware/config.h` and fill in your credentials
