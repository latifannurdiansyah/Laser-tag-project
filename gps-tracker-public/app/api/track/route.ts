export const dynamic = 'force-dynamic';
import { NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function POST(req) {
  try {
    const body = await req.json();
    const { deviceId, lat, lng, alt, sats } = body;

    if (!lat || !lng) {
      return NextResponse.json({ error: "Data Latitude atau Longitude tidak ditemukan" }, { status: 400 });
    }

    const newLocation = await prisma.gpsData.create({
      data: {
        deviceId: deviceId || 'Heltec-1',
        latitude: parseFloat(lat),
        longitude: parseFloat(lng),
        altitude: alt ? parseFloat(alt) : null,
        satellites: sats ? parseInt(sats) : null,
      },
    });

    return NextResponse.json({ 
      message: "Data berhasil disimpan", 
      data: newLocation 
    }, { status: 201 });
  } catch (error) {
    return NextResponse.json({ error: "Gagal menyimpan data ke database", details: error.message }, { status: 500 });
  }
}