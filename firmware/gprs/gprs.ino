// heltec_lora_sdcard_tft_gps_gprs_miles_irremote_immidiate_pps.ino
// Full integration: GPS + TFT + LoRaWAN + GPRS + SD + Serial + IR + PPS + Local Time (Indonesia)

#include "config.h"

// ==============================
// Global Shared Data
// ==============================

TinyGPSPlus GPS;
HardwareSerial sim900ASerial(2);
TinyGsm modem(sim900ASerial);
TinyGsmClient client(modem);

SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
LoRaWANNode node(&radio, &Region, subBand);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 framebuffer = GFXcanvas16(TFT_WIDTH, TFT_HEIGHT);

GpsData g_gpsData = { false, 0, 0, 0, 0, 0, 0, 0, false };
LoraStatus g_loraStatus = { "IDLE", 0, 0.0f, false };
GprsStatus g_gprsStatus = { "IDLE", false, 0 };
IrHitData g_irHitData = { 0, 0, 0, false, 0 };

// Removed: g_irLoraPending, g_irGprsPending, xIrSendMutex (no longer needed)

volatile bool ppsFlag = false;
volatile bool initialTimeSyncDone = false;

TftPageData g_TftPageData[TFT_MAX_PAGES] = {
    {"LoRaWAN Status", "-", "-", "-", "-", "-"},
    {"GPS Status", "Lat: -", "Long: -", "Alt: -", "Sat: -", "Time: --:--:--"},
    {"GPRS/Firebase", "Conn: NO", "IP: -", "Last: -", "Resp: -", "Evt: IDLE"},
    {"IR Disabled", "IR not active", "-", "-", "-", "-"}
};

// ==============================
// FreeRTOS Handles
// ==============================
SemaphoreHandle_t xGpsMutex = NULL;
SemaphoreHandle_t xLoraMutex = NULL;
SemaphoreHandle_t xGprsMutex = NULL;
SemaphoreHandle_t xIrMutex = NULL;
// xIrSendMutex removed
SemaphoreHandle_t xTftMutex = NULL;
QueueHandle_t xLogQueue = NULL;

TaskHandle_t xGpsTaskHandle = NULL;
TaskHandle_t xLoraTaskHandle = NULL;
TaskHandle_t xGprsTaskHandle = NULL;
TaskHandle_t xTftTaskHandle = NULL;
TaskHandle_t xSdTaskHandle = NULL;
TaskHandle_t xIrTaskHandle = NULL;

// ==============================
// Forward Declarations
// ==============================
void gpsTask(void *pv);
void loraTask(void *pv);
void gprsTask(void *pv);
void tftTask(void *pv);
void sdCardTask(void *pv);
void irTask(void *pv);
void logToSd(const String &msg);
String formatLogLine();

void ppsInterrupt();
void setSystemTimeFromGPS();
void fineTuneSystemTime();

// ==============================
// Helper: Build Payload (LoRa/GPRS — NO local time)
// ==============================
void buildDataPayload(DataPayload &payload) {
    payload.address_id = 0x12345678;
    payload.sub_address_id = random(0, 256);
    payload.status = 1;

    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        payload.lat = g_gpsData.lat;
        payload.lng = g_gpsData.lng;
        payload.alt = g_gpsData.alt;
        xSemaphoreGive(xGpsMutex);
    } else {
        payload.lat = payload.lng = payload.alt = 0.0f;
    }

    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        payload.shooter_address_id = g_irHitData.shooter_address_id;
        payload.shooter_sub_address_id = g_irHitData.shooter_sub_address_id;
        payload.irHitLocation = g_irHitData.irHitLocation;
        xSemaphoreGive(xIrMutex);
    } else {
        payload.shooter_address_id = 0;
        payload.shooter_sub_address_id = 0;
        payload.irHitLocation = 0;
    }
}

// ==============================
// LoRa Immediate Send Function - Disabled (IR not used)
// ==============================
/*
void sendImmediateLoRaPayload() {
    DataPayload payload;
    buildDataPayload(payload);

    uint8_t buffer[sizeof(DataPayload)];
    memcpy(buffer, &payload, sizeof(DataPayload));

    int16_t state = node.sendReceive(buffer, sizeof(buffer));
    String eventStr;
    float snr = 0.0f;
    int16_t rssi = 0;

    if (state < RADIOLIB_ERR_NONE) {
        eventStr = "TX_FAIL";
    } else {
        if (state > 0) {
            snr = radio.getSNR();
            rssi = radio.getRSSI();
            eventStr = "TX+RX_OK";
        } else {
            eventStr = "TX_OK";
        }
    }

    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_loraStatus.lastEvent = eventStr;
        g_loraStatus.snr = snr;
        g_loraStatus.rssi = rssi;
        xSemaphoreGive(xLoraMutex);
    }

    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[0].rows[1] = "Event: " + eventStr;
        g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
        g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
        xSemaphoreGive(xTftMutex);
    }

    logToSd("IMMEDIATE LoRa: " + eventStr + " | Alt: " + String(payload.alt));
}
*/

// ==============================
// GPRS Immediate Send Function - Disabled (IR not used)
// ==============================
/*
void sendImmediateGprsPayload() {
    DataPayload payload;
    buildDataPayload(payload);

    String event = "HTTP_FAIL";
    String responseLine = "-";

    // Build JSON payload for Firebase
    String jsonPayload = "{";
    jsonPayload += "\"lat\":" + String(payload.lat, 6) + ",";
    jsonPayload += "\"lng\":" + String(payload.lng, 6) + ",";
    jsonPayload += "\"alt\":" + String(payload.alt, 1) + ",";
    jsonPayload += "\"timestamp\":" + String(millis()/1000);
    jsonPayload += "}";

    if (client.connect(SERVER, SERVER_PORT)) {
        String url = String(RESOURCE) + "?auth=" + FIREBASE_SECRET;
        client.print(String("POST ") + url + " HTTP/1.1\r\n");
        client.print(String("Host: ") + SERVER + "\r\n");
        client.print("Content-Type: application/json\r\n");
        client.print("Content-Length: " + String(jsonPayload.length()) + "\r\n");
        client.print("Connection: close\r\n\r\n");
        client.print(jsonPayload);

        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 5000) break;
        }
        if (client.available()) {
            responseLine = client.readStringUntil('\n');
            if (responseLine.indexOf("200") >= 0 || responseLine.indexOf("OK") >= 0) {
                event = "FIREBASE_OK";
            } else {
                event = "FIREBASE_FAIL";
            }
        }
        client.stop();
    } else {
        logToSd("IMMEDIATE GPRS: HTTP connect failed");
    }

    logToSd("IMMEDIATE GPRS: " + event + " | Resp: " + responseLine);
    if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
        g_TftPageData[2].rows[5] = "Evt: " + event;
        xSemaphoreGive(xTftMutex);
    }
}
*/

// ==============================
// Setup
// ==============================
void setup()
{
    pinMode(Vext_Ctrl, OUTPUT);
    digitalWrite(Vext_Ctrl, HIGH);
    delay(100);
    pinMode(LED_K, OUTPUT);
    digitalWrite(LED_K, HIGH);

    // GNSS reset & PPS
    pinMode(GNSS_RSTPin, OUTPUT);
    digitalWrite(GNSS_RSTPin, HIGH);
    pinMode(GNSS_PPS_Pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(GNSS_PPS_Pin), ppsInterrupt, RISING);

    Serial.begin(115200);
    Serial.println("\n\n✅ Heltec Full System (LoRa+GPRS+IR+PPS+LocalTime) Started!");

    Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GNSS module powered.");

    // GPRS reset
    pinMode(GPRS_RST_PIN, OUTPUT);
    digitalWrite(GPRS_RST_PIN, HIGH);
    delay(200);
    digitalWrite(GPRS_RST_PIN, LOW);
    delay(200);
    digitalWrite(GPRS_RST_PIN, HIGH);
    delay(1000);
    sim900ASerial.begin(57600, SERIAL_8N1, GPRS_RX_PIN, GPRS_TX_PIN);
    delay(1000);

    // TFT
    tft.initR(INITR_MINI160x80);
    tft.setRotation(1);
    tft.fillScreen(ST7735_BLACK);

    // IR - Disabled
    // IrReceiver.begin(IR_RECEIVE_PIN, false);
    // Serial.println("IR Receiver Ready");

    // Mutexes
    xGpsMutex = xSemaphoreCreateMutex();
    xLoraMutex = xSemaphoreCreateMutex();
    xGprsMutex = xSemaphoreCreateMutex();
    xIrMutex = xSemaphoreCreateMutex();
    // xIrSendMutex removed
    xTftMutex = xSemaphoreCreateMutex();

    xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(String *));
    if (!xLogQueue) {
        Serial.println("❌ Log queue failed!");
        while (1) delay(1000);
    }

    // Tasks
    xTaskCreatePinnedToCore(gpsTask, "GPS", 4096, NULL, 2, &xGpsTaskHandle, 1);
    xTaskCreatePinnedToCore(loraTask, "LoRa", 8192, NULL, 2, &xLoraTaskHandle, 1);
    xTaskCreatePinnedToCore(gprsTask, "GPRS", 8192, NULL, 2, &xGprsTaskHandle, 1);
    xTaskCreatePinnedToCore(tftTask, "TFT", 4096, NULL, 1, &xTftTaskHandle, 0);
    xTaskCreatePinnedToCore(sdCardTask, "SD", 4096, NULL, 1, &xSdTaskHandle, 0);
    // xTaskCreatePinnedToCore(irTask, "IR", 4096, NULL, 3, &xIrTaskHandle, 1);  // Disabled - IR not used

    logToSd("=== System Boot (LoRa+GPRS+IR+PPS+LocalTime) ===");
}

void loop() {
    delay(1000);
}

// ==============================
// PPS Interrupt
// ==============================
void ppsInterrupt() {
    ppsFlag = true;
}

// ==============================
// Time Sync Helpers
// ==============================
void setSystemTimeFromGPS() {
    if (!GPS.date.isValid() || !GPS.time.isValid()) return;

    struct tm timeinfo = {0};
    timeinfo.tm_year = GPS.date.year() - 1900;
    timeinfo.tm_mon  = GPS.date.month() - 1;
    timeinfo.tm_mday = GPS.date.day();
    timeinfo.tm_hour = GPS.time.hour();
    timeinfo.tm_min  = GPS.time.minute();
    timeinfo.tm_sec  = GPS.time.second();
    time_t t = mktime(&timeinfo);

    struct timeval tv = { t, 0 };
    settimeofday(&tv, NULL);
    initialTimeSyncDone = true;
    Serial.println("✅ System time synced from GNSS (UTC)");
}

void fineTuneSystemTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    Serial.println("⏱️  System time fine-tuned via PPS (UTC)");
}

// ==============================
// GPS Task
// ==============================
void gpsTask(void *pv) {
    uint32_t lastGNSSDataMillis = 0;
    for (;;) {
        while (Serial1.available() > 0) {
            if (GPS.encode(Serial1.read())) {
                lastGNSSDataMillis = millis();
            }
        }

        if (!initialTimeSyncDone && GPS.date.isValid() && GPS.time.isValid()) {
            setSystemTimeFromGPS();
        }

        noInterrupts();
        if (ppsFlag) {
            fineTuneSystemTime();
            ppsFlag = false;
        }
        interrupts();

        bool hasTime = GPS.time.isValid();
        bool hasLoc = GPS.location.isValid();
        uint8_t sats = GPS.satellites.value();

        uint8_t localHour = 0, localMin = 0, localSec = 0;
        float lon = hasLoc ? GPS.location.lng() : 0.0f;
        if (hasTime) {
            int utcOffset = getUtcOffsetFromLongitude(lon);
            localHour = (GPS.time.hour() + utcOffset + 24) % 24;
            localMin = GPS.time.minute();
            localSec = GPS.time.second();
        }

        if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_gpsData.valid = hasTime || hasLoc;
            g_gpsData.hasLocation = hasLoc;
            if (hasLoc) {
                g_gpsData.lat = GPS.location.lat();
                g_gpsData.lng = lon;
                g_gpsData.alt = GPS.altitude.meters();
            } else {
                g_gpsData.lat = g_gpsData.lng = g_gpsData.alt = 0.0f;
            }
            g_gpsData.satellites = sats;
            g_gpsData.hour = localHour;
            g_gpsData.minute = localMin;
            g_gpsData.second = localSec;
            xSemaphoreGive(xGpsMutex);
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            g_TftPageData[1].rows[0] = "GPS Status";
            g_TftPageData[1].rows[1] = hasLoc ? ("Lat: " + String(g_gpsData.lat, 6)) : "Lat: -";
            g_TftPageData[1].rows[2] = hasLoc ? ("Long: " + String(g_gpsData.lng, 6)) : "Long: -";
            g_TftPageData[1].rows[3] = hasLoc ? ("Alt: " + String(g_gpsData.alt, 1) + " m") : "Alt: -";
            g_TftPageData[1].rows[4] = "Sat: " + String(sats);

            char timeBuf[12];
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                     hasTime ? localHour : 0,
                     hasTime ? localMin : 0,
                     hasTime ? localSec : 0);
            g_TftPageData[1].rows[5] = "Time: " + String(timeBuf);
            xSemaphoreGive(xTftMutex);
        }

        if (millis() - lastGNSSDataMillis > 60000) {
            Serial.println("⚠️  No GNSS data for >60s!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ==============================
// IR Task — DISABLED
// ==============================
/*
void irTask(void *pv) {
    for (;;) {
        if (IrReceiver.decode()) {
            uint8_t hitLocation = 0;
            uint32_t shooterAddr = 0;
            uint8_t shooterSub = 0;

            if (IrReceiver.decodedIRData.protocol == NEC) {
                shooterAddr = IrReceiver.decodedIRData.address;
                shooterSub = IrReceiver.decodedIRData.command;
                hitLocation = 1;

                String logMsg = "IR HIT (VEST) | Addr: 0x" + String(shooterAddr, HEX) +
                                " | Cmd: 0x" + String(shooterSub, HEX);
                logToSd(logMsg);
                Serial.println(logMsg);
            }

            if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_irHitData.shooter_address_id = shooterAddr;
                g_irHitData.shooter_sub_address_id = shooterSub;
                g_irHitData.irHitLocation = hitLocation;
                g_irHitData.valid = (hitLocation > 0);
                g_irHitData.timestamp = millis();
                xSemaphoreGive(xIrMutex);
            }

            if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[3].rows[0] = "IR Hit Info";
                if (hitLocation == 1) {
                    g_TftPageData[3].rows[1] = "Shooter: 0x" + String(shooterAddr, HEX);
                    g_TftPageData[3].rows[2] = "Sub: 0x" + String(shooterSub, HEX);
                    g_TftPageData[3].rows[3] = "Loc: VEST";
                    char timeBuf[12];
                    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu",
                             (millis() / 60000) % 60, (millis() / 1000) % 60);
                    g_TftPageData[3].rows[4] = "Time: " + String(timeBuf);
                } else {
                    g_TftPageData[3].rows[1] = "Shooter: -";
                    g_TftPageData[3].rows[2] = "Sub: -";
                    g_TftPageData[3].rows[3] = "Loc: NONE";
                    g_TftPageData[3].rows[4] = "Time: --:--";
                }
                xSemaphoreGive(xTftMutex);
            }

            // === IMMEDIATE RETRIED SENDS ===
            if (hitLocation == 1) {
                logToSd("IR: Triggering " + String(IR_IMMEDIATE_SEND_COUNT) + "x immediate sends (LoRa+GPRS)");

                // Send LoRa immediately, repeated
                for (uint8_t i = 0; i < IR_IMMEDIATE_SEND_COUNT; i++) {
                    sendImmediateLoRaPayload();
                    vTaskDelay(pdMS_TO_TICKS(100)); // small inter-attempt gap
                }

                // Send GPRS immediately, repeated
                for (uint8_t i = 0; i < IR_IMMEDIATE_SEND_COUNT; i++) {
                    sendImmediateGprsPayload();
                    vTaskDelay(pdMS_TO_TICKS(200)); // GPRS may need more time
                }
            }

            IrReceiver.resume();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
*/

// ==============================
// LoRaWAN Task (scheduled only)
// ==============================
void loraTask(void *pv) {
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        String err = "Radio init failed: " + stateDecode(state);
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        String err = "LoRaWAN init failed: " + stateDecode(state);
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    logToSd("Starting OTAA join...");
    Serial.println("Starting OTAA join...");

    uint8_t attempts = 0;
    while (attempts++ < MAX_JOIN_ATTEMPTS) {
        String msg = "OTAA attempt " + String(attempts);
        Serial.println(msg);
        logToSd(msg);

        state = node.activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            logToSd("✅ OTAA Join successful!");
            Serial.println("✅ OTAA Join successful!");
            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.joined = true;
                g_loraStatus.lastEvent = "JOINED";
                xSemaphoreGive(xLoraMutex);
            }
            break;
        } else {
            String err = "Join failed: " + stateDecode(state);
            Serial.println(err);
            logToSd(err);
            delay(JOIN_RETRY_DELAY_MS);
        }
    }

    if (!g_loraStatus.joined) {
        logToSd("❌ Max join attempts reached.");
        Serial.println("❌ Max join attempts reached.");
        vTaskDelete(NULL);
    }

    uint32_t lastUplink = 0;
    for (;;) {
        // Only scheduled uplinks now (no IR pending logic)
        if (millis() - lastUplink >= UPLINK_INTERVAL_MS) {
            DataPayload payload;
            buildDataPayload(payload);

            uint8_t buffer[sizeof(DataPayload)];
            memcpy(buffer, &payload, sizeof(DataPayload));

            state = node.sendReceive(buffer, sizeof(buffer));
            String eventStr;
            float snr = 0.0f;
            int16_t rssi = 0;

            if (state < RADIOLIB_ERR_NONE) {
                eventStr = "TX_FAIL";
            } else {
                if (state > 0) {
                    snr = radio.getSNR();
                    rssi = radio.getRSSI();
                    eventStr = "TX+RX_OK";
                } else {
                    eventStr = "TX_OK";
                }
            }

            if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_loraStatus.lastEvent = eventStr;
                g_loraStatus.snr = snr;
                g_loraStatus.rssi = rssi;
                xSemaphoreGive(xLoraMutex);
            }

            if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[0].rows[0] = "LoRaWAN Status";
                g_TftPageData[0].rows[1] = "Event: " + eventStr;
                g_TftPageData[0].rows[2] = "RSSI: " + String(rssi) + " dBm";
                g_TftPageData[0].rows[3] = "SNR: " + String(snr, 1) + " dB";
                g_TftPageData[0].rows[4] = "NA";
                g_TftPageData[0].rows[5] = "Joined: YES";
                xSemaphoreGive(xTftMutex);
            }

            logToSd(formatLogLine());
            lastUplink = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==============================
// GPRS Task (scheduled only)
// ==============================
void gprsTask(void *pv) {
    Serial.println("Initializing GPRS modem...");
    if (!modem.begin()) {
        String err = "GPRS init failed";
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    if (strlen(SIM_PIN) > 0 && !modem.simUnlock(SIM_PIN)) {
        String err = "Failed to unlock SIM";
        Serial.println(err);
        logToSd(err);
        vTaskDelete(NULL);
    }

    bool gprsConnected = false;
    for (int retry = 0; retry < 5; retry++) {
        if (modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
            gprsConnected = true;
            String ip = modem.getLocalIP();
            logToSd("GPRS: Connected persistently, IP=" + ip);
            Serial.println("GPRS: Connected persistently, IP=" + ip);
            break;
        }
        logToSd("GPRS: Connect retry " + String(retry + 1));
        delay(5000);
    }

    if (!gprsConnected) {
        logToSd("GPRS: Failed to establish persistent connection");
        Serial.println("GPRS: Failed to establish persistent connection");
    }

    uint32_t lastScheduledSend = 0;
    for (;;) {
        if (!gprsConnected || !modem.isGprsConnected()) {
            logToSd("GPRS: Connection lost — attempting reconnect");
            if (modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
                gprsConnected = true;
                String ip = modem.getLocalIP();
                logToSd("GPRS: Reconnected, IP=" + ip);
            } else {
                gprsConnected = false;
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }
        }

        // Send GPS data to Firebase every GPRS_INTERVAL_MS
        if (millis() - lastScheduledSend >= GPRS_INTERVAL_MS) {
            DataPayload payload;
            buildDataPayload(payload);

            String event = "HTTP_FAIL";
            String responseLine = "-";
            
            // Build JSON payload for Firebase
            String jsonPayload = "{";
            jsonPayload += "\"lat\":" + String(payload.lat, 6) + ",";
            jsonPayload += "\"lng\":" + String(payload.lng, 6) + ",";
            jsonPayload += "\"alt\":" + String(payload.alt, 1) + ",";
            jsonPayload += "\"timestamp\":" + String(millis()/1000);
            jsonPayload += "}";
            
            if (client.connect(SERVER, SERVER_PORT)) {
                String url = String(RESOURCE) + "?auth=" + FIREBASE_SECRET;
                client.print(String("POST ") + url + " HTTP/1.1\r\n");
                client.print(String("Host: ") + SERVER + "\r\n");
                client.print("Content-Type: application/json\r\n");
                client.print("Content-Length: " + String(jsonPayload.length()) + "\r\n");
                client.print("Connection: close\r\n\r\n");
                client.print(jsonPayload);

                unsigned long timeout = millis();
                while (client.available() == 0) {
                    if (millis() - timeout > 5000) break;
                }
                if (client.available()) {
                    responseLine = client.readStringUntil('\n');
                    if (responseLine.indexOf("200") >= 0 || responseLine.indexOf("OK") >= 0) {
                        event = "FIREBASE_OK";
                    } else {
                        event = "FIREBASE_FAIL";
                    }
                }
                client.stop();
            }

            logToSd("FIREBASE: " + event + " | Resp: " + responseLine + " | Data: " + jsonPayload);
            if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
                g_TftPageData[2].rows[1] = "Conn: YES";
                g_TftPageData[2].rows[2] = "IP: " + modem.getLocalIP();
                String displayResp = responseLine;
                if (displayResp.length() > 10) displayResp = displayResp.substring(0, 10);
                g_TftPageData[2].rows[4] = "Resp: " + displayResp;
                g_TftPageData[2].rows[5] = "Evt: " + event;
                xSemaphoreGive(xTftMutex);
            }

            lastScheduledSend = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==============================
// TFT Task
// ==============================
void tftTask(void *pv) {
    uint32_t pageSwitch = millis();
    uint8_t currentPage = 0;

    for (;;) {
        if (millis() - pageSwitch >= PAGE_SWITCH_MS) {
            currentPage = (currentPage + 1) % TFT_MAX_PAGES;
            pageSwitch = millis();
        }

        if (xSemaphoreTake(xTftMutex, MUTEX_TIMEOUT) == pdTRUE) {
            framebuffer.fillScreen(ST7735_BLACK);
            framebuffer.setTextColor(ST7735_WHITE);
            framebuffer.setTextSize(1);
            for (int i = 0; i < TFT_ROWS_PER_PAGE; i++) {
                int y = TFT_TOP_MARGIN + i * TFT_LINE_HEIGHT;
                if (y >= 80) break;
                framebuffer.setCursor(TFT_LEFT_MARGIN, y);
                framebuffer.print(g_TftPageData[currentPage].rows[i]);
            }
            xSemaphoreGive(xTftMutex);
            tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), TFT_WIDTH, TFT_HEIGHT);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==============================
// SD Card Task
// ==============================
void sdCardTask(void *pv) {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 8000000)) {
        Serial.println("SD Mount Failed!");
        vTaskDelete(NULL);
    }
    if (SD.exists("/tracker.log")) {
        SD.remove("/tracker.log");
        Serial.println("Old tracker.log deleted.");
    }

    File file;
    uint32_t lastWrite = 0;
    for (;;) {
        uint32_t now = millis();
        if (now - lastWrite >= SD_WRITE_INTERVAL_MS) {
            file = SD.open("/tracker.log", FILE_APPEND);
            if (file) {
                String *msg;
                while (xQueueReceive(xLogQueue, &msg, 0) == pdTRUE) {
                    file.println(*msg);
                    delete msg;
                }
                file.close();
                lastWrite = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==============================
// Logging Helpers
// ==============================
void logToSd(const String &msg) {
    String *copy = new String(msg);
    if (xQueueSendToBack(xLogQueue, &copy, pdMS_TO_TICKS(10)) != pdTRUE) {
        delete copy;
    }
}

String formatLogLine() {
    char timeBuf[9];
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                 g_gpsData.valid ? g_gpsData.hour : 0,
                 g_gpsData.valid ? g_gpsData.minute : 0,
                 g_gpsData.valid ? g_gpsData.second : 0);
        xSemaphoreGive(xGpsMutex);
    } else {
        strcpy(timeBuf, "00:00:00");
    }

    String event = "IDLE";
    float snr = 0.0f;
    int16_t rssi = 0;
    if (xSemaphoreTake(xLoraMutex, MUTEX_TIMEOUT) == pdTRUE) {
        event = g_loraStatus.lastEvent;
        snr = g_loraStatus.snr;
        rssi = g_loraStatus.rssi;
        xSemaphoreGive(xLoraMutex);
    }

    char latBuf[20] = "-", lonBuf[20] = "-", altBuf[20] = "-";
    if (xSemaphoreTake(xGpsMutex, MUTEX_TIMEOUT) == pdTRUE) {
        if (g_gpsData.valid && g_gpsData.hasLocation) {
            dtostrf(g_gpsData.lat, 1, 6, latBuf);
            dtostrf(g_gpsData.lng, 1, 6, lonBuf);
            dtostrf(g_gpsData.alt, 1, 1, altBuf);
        }
        xSemaphoreGive(xGpsMutex);
    }

    uint8_t irLoc = 0;
    if (xSemaphoreTake(xIrMutex, MUTEX_TIMEOUT) == pdTRUE) {
        irLoc = g_irHitData.irHitLocation;
        xSemaphoreGive(xIrMutex);
    }

    char logLine[180];
    snprintf(logLine, sizeof(logLine),
             "%s,%s,%.1f,%d,%s,%s,%s,IR:%d",
             timeBuf, event.c_str(), snr, rssi, latBuf, lonBuf, altBuf, irLoc);
    return String(logLine);
}

String stateDecode(const int16_t result) {
    switch (result) {
        case RADIOLIB_ERR_NONE: return "ERR_NONE";
        case RADIOLIB_ERR_CHIP_NOT_FOUND: return "ERR_CHIP_NOT_FOUND";
        case RADIOLIB_ERR_PACKET_TOO_LONG: return "ERR_PACKET_TOO_LONG";
        case RADIOLIB_ERR_RX_TIMEOUT: return "ERR_RX_TIMEOUT";
        case RADIOLIB_ERR_MIC_MISMATCH: return "ERR_MIC_MISMATCH";
        case RADIOLIB_ERR_INVALID_FREQUENCY: return "ERR_INVALID_FREQUENCY";
        case RADIOLIB_ERR_NETWORK_NOT_JOINED: return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
        case RADIOLIB_ERR_DOWNLINK_MALFORMED: return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
        case RADIOLIB_ERR_NO_RX_WINDOW: return "RADIOLIB_ERR_NO_RX_WINDOW";
        case RADIOLIB_ERR_UPLINK_UNAVAILABLE: return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
        case RADIOLIB_ERR_NO_JOIN_ACCEPT: return "RADIOLIB_ERR_NO_JOIN_ACCEPT";
        case RADIOLIB_LORAWAN_NEW_SESSION: return "RADIOLIB_LORAWAN_NEW_SESSION";
        case RADIOLIB_LORAWAN_SESSION_RESTORED: return "RADIOLIB_LORAWAN_SESSION_RESTORED";
        default: return "Unknown error (" + String(result) + ")";
    }
}