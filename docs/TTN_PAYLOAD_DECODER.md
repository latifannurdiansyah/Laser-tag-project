# TTN Payload Decoder Configuration

## Binary Payload Structure (32 bytes)

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | uint32 | address_id | Device identifier (0=P1, 1=P2, etc) |
| 4 | 1 | uint8 | sub_address_id | IR Command/Button received |
| 5 | 4 | uint32 | shooter_address_id | IR Address (shooter ID) |
| 9 | 1 | uint8 | shooter_sub_address_id | IR hit flag (1=hit) |
| 10 | 1 | uint8 | status | 0=inactive, 1=hit |
| 11 | 4 | float | lat | GPS latitude |
| 15 | 4 | float | lng | GPS longitude |
| 19 | 4 | float | alt | GPS altitude |
| 23 | 2 | uint16 | battery_mv | Battery voltage (mV) |
| 25 | 1 | uint8 | satellites | GPS satellites locked |
| 26 | 2 | int16 | rssi | LoRa RSSI (dBm) |
| 28 | 1 | int8 | snr | LoRa SNR (dB) |
| 29 | 3 | - | - | Padding |

## TTN Console Configuration

### 1. Payload Format (Uplink)
```
https://laser-tag-project.vercel.app/api/ttn
```

### 2. Payload Decoder (JavaScript)

**SALIN KODE DIBAWAH INI ke TTN Console → Application → Payload Formats → Uplink:**

```javascript
function decodeUplink(input) {
    var bytes = input.bytes;
    var data = {};
    
    // Helper: Read Float32 Little Endian
    function readFloat32LE(b, o) {
        var buffer = new ArrayBuffer(4);
        var view = new DataView(buffer);
        for (var i = 0; i < 4; i++) {
            view.setUint8(i, b[o + i]);
        }
        return view.getFloat32(0, true);
    }
    
    // Helper: Read UInt16 Little Endian
    function readUInt16LE(b, o) {
        return b[o] | (b[o + 1] << 8);
    }
    
    // Helper: Read Int16 Little Endian (signed)
    function readInt16LE(b, o) {
        var val = b[o] | (b[o + 1] << 8);
        return val > 32767 ? val - 65536 : val;
    }
    
    // GPS Data (offset 11-26)
    data.lat = readFloat32LE(bytes, 11);
    data.lng = readFloat32LE(bytes, 15);
    data.alt = readFloat32LE(bytes, 19);
    
    // IR Data (offset 0-10)
    data.address_id = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    data.sub_address_id = bytes[4];              // IR Command (button)
    data.shooter_address_id = (bytes[8] << 24) | (bytes[7] << 16) | (bytes[6] << 8) | bytes[5];  // Shooter ID
    data.shooter_sub_address_id = bytes[9];     // IR hit flag
    data.status = bytes[10];                    // 0=no hit, 1=hit
    
    // Device Info
    data.battery_mv = readUInt16LE(bytes, 23);
    data.satellites = bytes[25];
    data.rssi = readInt16LE(bytes, 26);
    data.snr = bytes[28];
    
    // Create Device ID
    var playerNum = data.address_id + 1;
    data.deviceId = "Heltec-P" + playerNum.toString();
    
    // Create IR Status String - PENTING!
    // Jika status=1 (ada HIT), tampilkan: HIT: 0xSHOOTER-0xCOMMAND
    // Jika status=0 (tidak ada HIT), tampilkan: -
    if (data.status === 1) {
        data.irStatus = "HIT: 0x" + data.shooter_address_id.toString(16).toUpperCase() + "-0x" + data.sub_address_id.toString(16).toUpperCase();
    } else {
        data.irStatus = "-";
    }
    
    // Return data untuk webhook - PENTING: Semua field ini akan dikirim ke Vercel!
    return {
        data: {
            deviceId: data.deviceId,
            lat: data.lat,
            lng: data.lng,
            alt: data.alt,
            irStatus: data.irStatus,           // <-- Field ini wajib ada!
            address: data.shooter_address_id,  // <-- Untuk backup parsing
            command: data.sub_address_id,       // <-- Untuk backup parsing
            battery: data.battery_mv,
            satellites: data.satellites,
            rssi: data.rssi,
            snr: data.snr
        },
        warnings: [],
        errors: []
    };
}
```

### 3. Webhook Configuration

1. Go to **TTN Console** → Your Application → **Integrations** → **Webhooks**
2. Click **Add Webhook** → **Custom Webhook**
3. Fill in:
   - **Webhook ID**: `vercel-gps-tracker`
   - **Webhook URL**: `https://laser-tag-project.vercel.app/api/ttn`
   - **Method**: `POST`
   - **Uplink message**: ✅ Enabled
4. Click **Add Webhook**

### 4. Downlink Payload Encoder (Optional)

If you want to send commands to the device:

```javascript
function encodeDownlink(input) {
    var bytes = [];
    
    bytes[0] = (input.deviceId >> 0) & 0xFF;
    bytes[1] = (input.deviceId >> 8) & 0xFF;
    bytes[2] = (input.deviceId >> 16) & 0xFF;
    bytes[3] = (input.deviceId >> 24) & 0xFF;
    bytes[4] = input.command || 0x00;
    bytes[5] = 0x00;
    bytes[6] = 0x00;
    bytes[7] = 0x00;
    bytes[8] = 0x00;
    bytes[9] = 0x00;
    bytes[10] = 0x00;
    
    for (var i = 11; i < 32; i++) {
        bytes[i] = 0x00;
    }
    
    return {
        bytes: bytes,
        fPort: 1,
        confirmed: false
    };
}
```

## Testing

1. Deploy the web app to Vercel first
2. Configure TTN webhook with your Vercel URL
3. Power on your Heltec tracker
4. Watch the dashboard at `https://laser-tag-project.vercel.app/dashboard`
5. Data should appear within 10 seconds (uplink interval)
6. **Test IR**: Send IR signal to receiver, then check dashboard - should show `HIT (0xABCD - 0x12)`
