# SIM900A GPRS Implementation Guide

## Overview
Firmware ini mengirim data GPS ke Vercel API via GPRS (Indosat) menggunakan SIM900A module.

## Problem: HTTPS vs HTTP
**SIM900A tidak support HTTPS (SSL/TLS) secara native.**

Vercel secara default menggunakan HTTPS dan force redirect dari HTTP ke HTTPS, yang tidak bisa di-handle oleh SIM900A.

## Solusi yang Tersedia:

### Opsi 1: HTTP Endpoint (Dicoba pertama)
- Endpoint: `/api/track-http`
- Port: 80
- Status: Mungkin tidak berfungsi karena Vercel force HTTPS redirect

### Opsi 2: Gunakan Proxy Server (Rekomendasi)

#### A. Deploy ke VPS/Cloud dengan HTTP support:
```javascript
// express-proxy.js
const express = require('express');
const app = express();
const { createProxyMiddleware } = require('http-proxy-middleware');

app.use('/api/track', createProxyMiddleware({
  target: 'https://laser-tag-project.vercel.app',
  changeOrigin: true,
  secure: false
}));

app.listen(80, () => {
  console.log('HTTP Proxy running on port 80');
});
```

#### B. Gunakan layanan gratis seperti:
- **Webhook.site** - untuk testing
- **RequestBin** - untuk testing  
- **ThingSpeak** - IoT platform dengan HTTP support

### Opsi 3: Update Firmware untuk ThingSpeak

Jika Vercel tidak bisa HTTP, gunakan ThingSpeak sebagai intermediate:

```cpp
// Ganti API configuration:
#define API_URL "api.thingspeak.com"
#define API_PATH "/update"
#define API_PORT 80

// API Key dari ThingSpeak
#define THINGSPEAK_API_KEY "YOUR_API_KEY"
```

### Opsi 4: Deploy Next.js ke Server Sendiri

Deploy `my-gps-tracker` ke:
- **Railway.app**
- **Render.com**  
- **DigitalOcean**
- **AWS EC2**

Dengan konfigurasi HTTP enabled.

## Cara Test:

### 1. Test endpoint HTTP:
```bash
curl -X POST http://laser-tag-project.vercel.app/api/track-http \
  -H "Content-Type: application/json" \
  -d '{"source":"gprs","id":"Heltec-P1","lat":-6.123456,"lng":106.123456}'
```

Jika mendapat redirect (301/307), berarti perlu proxy.

### 2. Test dengan Proxy lokal:
```bash
# Install localtunnel
npm install -g localtunnel

# Jalankan proxy
lt --port 3000 --subdomain mygps

# Update firmware ke domain localtunnel
```

## Rekomendasi Saya:

**Untuk production yang paling mudah:**

1. **Gunakan ThingSpeak** - support HTTP, gratis, dashboard included
2. **Webhook ke Vercel** - setup webhook di ThingSpeak yang forward ke Vercel API

Atau

1. **Deploy ke Railway.app** - support HTTP, free tier available
2. Update firmware ke Railway domain

## Langkah Selanjutnya:

Silakan test dulu endpoint HTTP:
```bash
curl -v http://laser-tag-project.vercel.app/api/track-http \
  -H "Content-Type: application/json" \
  -d '{"source":"gprs","id":"Heltec-P1","lat":-6.123456,"lng":106.123456}'
```

Jika mendapat error redirect, saya akan setup proxy server atau integrasi ThingSpeak.

## Database Schema:

Data akan disimpan dengan `source: "gprs"` di tabel `gpsLog`.

```sql
-- Filter data GPRS:
SELECT * FROM gpsLog WHERE source = 'gprs';
```
