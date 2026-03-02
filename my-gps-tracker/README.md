# GPS Tracker Dashboard

Next.js 16 + Prisma + PostgreSQL dashboard untuk tracking data GPS dari ESP32 firmware.

## Features

- **Live Data**: Polling every 2 seconds
- **Dual Source Support**: WiFi HTTP + TTN LoRaWAN
- **Extended Fields**: Device, Lat, Lng, Alt, IR Status, Battery, Satellites, RSSI, SNR
- **Visual Indicators**: Battery level, signal strength, IR hit alerts
- **Responsive Design**: Mobile + Desktop support

## Tech Stack

- Next.js 16 (App Router)
- Prisma ORM
- PostgreSQL (Vercel Postgres / Neon)
- Tailwind CSS

## Setup

```bash
npm install
npx prisma generate
npx prisma db push
npm run dev
```

## Environment Variables

```env
DATABASE_URL="postgresql://..."
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/track` | POST | Receive GPS data from ESP32 (WiFi) |
| `/api/ttn` | POST | Receive GPS data from TTN webhook (LoRaWAN) |
| `/api/logs` | GET | Fetch all logs (polled by dashboard) |
| `/api/logs` | DELETE | Clear all logs |
| `/api/random` | POST | Generate random test data |

## Data Schema

```prisma
model GpsLog {
  id          Int      @id @default(autoincrement())
  source      String   @default("wifi")  // "ttn" or "wifi"
  deviceId    String
  latitude    Float
  longitude   Float
  altitude    Float?
  irStatus    String?  @default("-")
  battery     Int?     // millivolts
  satellites  Int?
  rssi        Int?
  snr         Float?
  rawPayload  Json?
  createdAt   DateTime @default(now())
}
```

## Firmware Integration

### WiFi Mode (HTTP)
- URL: `https://laser-tag-project.vercel.app/api/track`
- Method: `POST`
- Body:
```json
{
  "source": "wifi",
  "id": "Player 1",
  "lat": -6.2088,
  "lng": 106.8456,
  "alt": 10.5,
  "irStatus": "HIT: 0xABCD-0x12",
  "battery": 4123,
  "satellites": 8,
  "rssi": -75,
  "snr": 7.5
}
```

### LoRaWAN Mode (TTN)
See `../../docs/TTN_PAYLOAD_DECODER.md` for TTN configuration.

## Deployment

1. Push to GitHub
2. Import in Vercel
3. Add `DATABASE_URL` environment variable
4. Deploy
5. Configure TTN webhook to `https://your-vercel-app.vercel.app/api/ttn`
