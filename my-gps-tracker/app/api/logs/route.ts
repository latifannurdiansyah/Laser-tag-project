import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function GET() {
  try {
    console.log('[API] Fetching logs from database...')
    
    // Get all logs with available fields
    const logs = await prisma.$queryRaw`
      SELECT 
        id, 
        "deviceId", 
        latitude, 
        longitude,
        "createdAt",
        COALESCE("irStatus", '-') as "irStatus",
        "source",
        altitude,
        battery,
        satellites,
        rssi,
        snr
      FROM "gps_logs" 
      ORDER BY "createdAt" DESC 
      LIMIT 100
    `
    
    console.log('[API] Logs found:', (logs as any[]).length)
    return NextResponse.json(logs)
  } catch (error: any) {
    console.error('[API] Error fetching logs:', error)
    // Fallback untuk database lama
    try {
      const fallback = await prisma.gpsLog.findMany({
        select: {
          id: true,
          deviceId: true,
          latitude: true,
          longitude: true,
          irStatus: true,
          createdAt: true
        },
        orderBy: { createdAt: 'desc' },
        take: 100
      })
      return NextResponse.json(fallback)
    } catch (fallbackError: any) {
      return NextResponse.json({ error: error.message || 'Unknown error' }, { status: 500 })
    }
  }
}

export async function DELETE() {
  try {
    await prisma.gpsLog.deleteMany()
    console.log('[API] All logs deleted')
    return NextResponse.json({ message: 'All logs deleted' })
  } catch (error: any) {
    console.error('[API] Error deleting logs:', error)
    return NextResponse.json({ error: error.message || 'Unknown error' }, { status: 500 })
  }
}
