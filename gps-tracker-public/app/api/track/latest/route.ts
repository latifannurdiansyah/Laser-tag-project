export const dynamic = 'force-dynamic';
import { NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function GET(req) {
  try {
    const { searchParams } = new URL(req.url);
    const deviceId = searchParams.get('deviceId');

    const latestData = await prisma.gpsData.findFirst({
      orderBy: { createdAt: 'desc' },
      where: deviceId ? { deviceId: deviceId } : undefined,
    });

    if (!latestData) {
      return NextResponse.json({ error: "Tidak ada data GPS ditemukan" }, { status: 404 });
    }

    return NextResponse.json({ 
      success: true, 
      data: latestData 
    }, { status: 200 });
  } catch (error) {
    return NextResponse.json({ error: "Gagal mengambil data", details: error.message }, { status: 500 });
  }
}
