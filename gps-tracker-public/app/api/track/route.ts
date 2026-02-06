import { NextResponse } from 'next/server';
import { PrismaClient } from '@prisma/client';

// Inisialisasi Prisma untuk koneksi ke Neon
const prisma = new PrismaClient();

export async function POST(request: Request) {
  try {
    // 1. Mengambil data JSON yang dikirim oleh Heltec/Microcontroller
    const body = await request.json();
    const { lat, lng } = body;

    // 2. Validasi sederhana: pastikan data lat dan lng ada
    if (lat === undefined || lng === undefined) {
      return NextResponse.json(
        { error: "Data Latitude atau Longitude tidak ditemukan" }, 
        { status: 400 }
      );
    }

    // 3. Simpan data ke tabel GpsData di Neon
    const dataBaru = await prisma.gpsData.create({
      data: {
        latitude: parseFloat(lat),
        longitude: parseFloat(lng),
      },
    });

    // 4. Beri respon sukses ke Heltec
    return NextResponse.json({ 
      success: true, 
      message: "Data berhasil mendarat di Neon!",
      id: dataBaru.id 
    });

  } catch (error) {
    console.error("Error API:", error);
    return NextResponse.json(
      { success: false, error: "Gagal menyimpan data ke database" }, 
      { status: 500 }
    );
  }
}