# TTN Payload Decoder Configuration

## Binary Payload Structure (32 bytes)

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | uint32 | address_id | Device identifier |
| 4 | 1 | uint8 | sub_address_id | IR Command received |
| 5 | 4 | uint32 | shooter_address_id | IR Address (shooter) |
| 9 | 1 | uint8 | shooter_sub_address_id | IR hit flag |
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

Copy this to your TTN Console → Application → Payload Formats → Uplink:

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
    data.shooter_sub_address_id = bytes[9];
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
5. Data should appear within 30 seconds (uplink interval)
