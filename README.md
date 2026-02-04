# Laser-tag-project

This project is a laser tag game system consisting of three main devices, namely weapons, helmets, and vests. These three devices are integrated to detect shots, process data, and send game results to the server in real time.  The system begins by activating all devices. Once the system is ready, players can shoot by pressing the push button on the gun. When the button is pressed, the infrared (IR) transmitter on the gun emits an infrared signal containing the shooter's ID data. This signal is directed at the opponent and can be received by the opponent's helmet or vest.  If the infrared signal is received by the helmet, the system will first check for protocol compliance using the NEC protocol. If the signal complies with the specified protocol, the helmet will read the shooter's ID data and send it to the vest using ESP-NOW communication. Next, the vest will forward the data to the LoRa Gateway via the LoRa network. However, if the received signal does not comply with the NEC protocol, the shot is considered invalid or missed.  If the infrared signal is received directly by the vest, the process is almost the same. The vest will verify the signal's compliance with the NEC protocol. If valid, the vest will read the shooter's ID data and add information about the location and time of the shot. The complete data is then sent to LoRa.

## GPS Tracking System

This project also includes a real-time GPS tracking system for monitoring device locations.

### Hardware
- **Heltec Wireless Tracker V1.2** (ESP32-based)
- **Components**: GPS module, TFT display (160x80), WiFi

### Firmware (`firmware/Heltec Wireless Tracker V1.2/GPS_TFT_Wifi_firebase/`)
- Reads GPS data using TinyGPS++ library
- Displays live GPS info on TFT display (Latitude, Longitude, Satellites, Altitude)
- Sends data to Firebase Realtime Database every 1 second
- Data path: `/gps_tracking` with fields: `latitude`, `longitude`, `altitude`, `satellites`

### Software Server (`software/server/`)
- **index.html**: Modern dashboard with dark theme, real-time table display, DevTools panel
- **index.js**: Firebase RTDB integration, DevTools for testing (random coords, simulate GPS, export/import JSON)
- **firebase.js**: Firebase SDK configuration

### Firebase Configuration
- Project ID: `gps-log-a1d90`
- Database URL: `https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app`

### Usage
1. Upload firmware to Heltec Wireless Tracker
2. Open `software/server/index.html` in browser
3. View real-time GPS tracking data on dashboard
