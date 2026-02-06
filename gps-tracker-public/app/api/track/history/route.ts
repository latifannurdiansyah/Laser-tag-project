export const dynamic = 'force-dynamic';
import { NextResponse } from 'next/server';
import { prisma } from '@/lib/prisma';

export async function GET(req) {
  try {
    const { searchParams } = new URL(req.url);
    const deviceId = searchParams.get('deviceId');
    const limit = parseInt(searchParams.get('limit') || '50');
    const offset = parseInt(searchParams.get('offset') || '0');

    const whereClause = deviceId ? { deviceId: deviceId } : {};

    const [data, total] = await Promise.all([
      prisma.gpsData.findMany({
        orderBy: { createdAt: 'desc' },
        where: whereClause,
        take: limit,
        skip: offset,
      }),
      prisma.gpsData.count({ where: whereClause }),
    ]);

    return NextResponse.json({ 
      success: true, 
      data: data,
      pagination: {
        total,
        limit,
        offset,
        hasMore: offset + limit < total
      }
    }, { status: 200 });
  } catch (error) {
    return NextResponse.json({ error: "Gagal mengambil riwayat data", details: error.message }, { status: 500 });
  }
}
