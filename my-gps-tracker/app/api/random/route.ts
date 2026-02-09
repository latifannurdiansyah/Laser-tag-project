import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

function generateRandomIndonesia(): { lat: number; lng: number } {
  const lat = -11.0 + Math.random() * 8.0
  const lng = 95.0 + Math.random() * 25.0
  return { lat, lng }
}

export async function POST() {
  try {
    const { lat, lng } = generateRandomIndonesia()
    console.log('[API] Generating random coords:', lat, lng)

    const log = await prisma.gpsLog.create({
      data: {
        deviceId: 'RANDOM-GEN',
        latitude: lat,
        longitude: lng
      }
    })

    console.log('[API] Random log created:', log.id)
    return NextResponse.json(log)
  } catch (error: any) {
    console.error('[API] Error generating random log:', error)
    return NextResponse.json({ error: error.message || 'Failed to generate random log' }, { status: 500 })
  }
}
