// TTN Payload Decoder (JavaScript)
// Paste this in TTN Console → Applications → Payload Formatters → Uplink

function decodeUplink(input) {
  var bytes = input.bytes;
  
  // Check if we have enough bytes (25 bytes expected)
  if (bytes.length < 25) {
    return {
      data: {
        error: "Invalid payload length: " + bytes.length
      },
      warnings: ["Expected 25 bytes, got " + bytes.length]
    };
  }

  // Parse DataPayload structure
  var data = {};
  
  // address_id (uint32_t, 4 bytes)
  data.device_id = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
  data.device_id_hex = "0x" + data.device_id.toString(16).toUpperCase();
  
  // sub_address_id (uint8_t, 1 byte) - IR Command
  data.ir_command = bytes[4];
  data.ir_command_hex = "0x" + bytes[4].toString(16).toUpperCase().padStart(2, '0');
  
  // shooter_address_id (uint32_t, 4 bytes) - IR Address
  data.shooter_id = (bytes[5] << 24) | (bytes[6] << 16) | (bytes[7] << 8) | bytes[8];
  data.shooter_id_hex = "0x" + data.shooter_id.toString(16).toUpperCase();
  
  // shooter_sub_address_id (uint8_t, 1 byte)
  data.shooter_sub_id = bytes[9];
  
  // status (uint8_t, 1 byte) - 0=normal, 1=hit detected
  data.status = bytes[10];
  data.hit_detected = (data.status === 1);
  
  // lat, lng, alt (3x float, 12 bytes total)
  // Convert 4 bytes to float (IEEE 754)
  function bytesToFloat(bytes, offset) {
    var bits = (bytes[offset] << 24) | (bytes[offset+1] << 16) | 
               (bytes[offset+2] << 8) | bytes[offset+3];
    var sign = (bits >>> 31 === 0) ? 1.0 : -1.0;
    var e = (bits >>> 23) & 0xff;
    var m = (e === 0) ? (bits & 0x7fffff) << 1 : (bits & 0x7fffff) | 0x800000;
    return sign * m * Math.pow(2, e - 150);
  }
  
  data.latitude = bytesToFloat(bytes, 11);
  data.longitude = bytesToFloat(bytes, 15);
  data.altitude = bytesToFloat(bytes, 19);
  
  // Add human-readable location
  if (data.latitude !== 0 && data.longitude !== 0) {
    data.location = data.latitude.toFixed(6) + ", " + data.longitude.toFixed(6);
  } else {
    data.location = "No GPS fix";
  }

  return {
    data: data
  };
}

// Example usage (for testing in TTN Console):
// Input (hex): 12345678 FF ABCD1234 01 01 40490FDB 40C90FDB 42C80000
// This represents:
// - device_id: 0x12345678
// - ir_command: 0xFF (255)
// - shooter_id: 0xABCD1234
// - shooter_sub_id: 1
// - status: 1 (hit detected)
// - lat: 3.141592
// - lng: 6.283185
// - alt: 100.0
