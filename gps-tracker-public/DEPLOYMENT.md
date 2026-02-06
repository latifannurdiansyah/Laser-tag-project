# Deployment Guide - GPS Tracker (Vercel + Neon PostgreSQL)

## Prerequisites

- GitHub account: **latifannurdiansyah02@gmail.com**
- Vercel account: **latifannurdiansyah03@gmail.com**
- Neon PostgreSQL database (already configured)

## Step 1: Push Code to GitHub

```bash
git add .
git commit -m "feat: prepare for Vercel deployment with Neon Postgres"
git push origin feat-1
```

## Step 2: Deploy ke Vercel

1. Buka https://vercel.com
2. Login dengan: **latifannurdiansyah03@gmail.com**
3. Klik **"Add New..."** → **"Project"**
4. Import repository **Laser-tag-project** (branch: `feat-1`)

## Step 3: Configure Environment Variables

Di Vercel Dashboard → Project Settings → **Environment Variables**, tambahkan:

| Key | Value |
|-----|-------|
| `DATABASE_URL` | `postgresql://neondb_owner:npg_1M0oseQTBUyi@ep-lively-star-a1aqeuvs-pooler.ap-southeast-1.aws.neon.tech/neondb?sslmode=require` |

**Penting**: Pastikan `DATABASE_URL` menggunakan `sslmode=require`

## Step 4: Configure Build Settings

Di Project Settings → **General**:

```
Build Command:     npx prisma generate && next build
Output Directory:  .next
Install Command:   npm install
```

## Step 5: Deploy & Get URL

1. Klik **"Deploy"** di Vercel
2. Tunggu build selesai (~2-3 menit)
3. Copy URL Vercel Anda (contoh): `https://gps-tracker-xyz.vercel.app`

## Step 6: Update Firmware ESP32

Edit file firmware yang Anda gunakan:

- `gps-tracker-public/heltec_postgres_wifi/heltec_postgres_wifi.ino`
- `gps-tracker-public/bareminimum_database_test/bareminimum_database_test.ino`

Ubah `serverName` dengan URL Vercel Anda:

```cpp
const char* serverName = "https://gps-tracker-xyz.vercel.app/api/track";
```

## Step 7: Testing

### Test API Endpoint

Buka browser dan akses:
```
https://[YOUR-VERCEL-URL].vercel.app/api/logs
```

Harus mengembalikan JSON array (mungkin kosong): `[]`

### Test Dashboard

```
https://[YOUR-VERCEL-URL].vercel.app/dashboard
```

### Test POST (dari ESP32)

Firmware akan mengirim data GPS ke:
```
POST https://[YOUR-VERCEL-URL].vercel.app/api/track
Body: {"id":"HELTEC-01","lat":-7.257472,"lng":112.752088}
```

## API Reference

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/track` | POST | Kirim data GPS dari ESP32 |
| `/api/logs` | GET | Ambil semua data GPS |
| `/api/logs` | DELETE | Hapus semua data |

## Troubleshooting

### Error: "Can't reach database"

1. Cek Environment Variables di Vercel
2. Pastikan `DATABASE_URL` sudah diset dengan benar
3. Rebuild project setelah add environment variable

### Error: "Prisma Client not generated"

Pastikan build command: `npx prisma generate && next build`

### Error: Connection timeout

- Neon free tier memiliki limit koneksi
- Tunggu beberapa menit lalu retry

## URLs After Deployment

| Environment | URL |
|-------------|-----|
| Dashboard | `https://gps-tracker-xxx.vercel.app/dashboard` |
| API Logs | `https://gps-tracker-xxx.vercel.app/api/logs` |
| API Track | `https://gps-tracker-xxx.vercel.app/api/track` |

## Next Steps

1. ✅ Deploy ke Vercel
2. ⬜ Update firmware dengan URL Vercel
3. ⬜ Test dengan ESP32 hardware
4. ⬜ (Optional) Setup custom domain
