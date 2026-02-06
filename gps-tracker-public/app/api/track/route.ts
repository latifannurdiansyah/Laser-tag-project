import { NextResponse } from 'next/server';
import { PrismaClient } from '@prisma/client';

const prisma = new PrismaClient();

export async function POST(req) {
  try {
    const body = await req.json();
    const { lat, lng } = body;

    if (!lat || !lng) {
      return NextResponse.json({ error: "Data Latitude atau Longitude tidak ditemukan" }, { status: 400 });
    }

    const newLocation = await prisma.location.create({
      data: {
        latitude: parseFloat(lat),
        longitude: parseFloat(lng),
      },
    });

    return NextResponse.json({ message: "Data berhasil disimpan", data: newLocation }, { status: 201 });
  } catch (error) {
    return NextResponse.json({ error: "Gagal menyimpan data ke database", details: error.message }, { status: 500 });
  }
}