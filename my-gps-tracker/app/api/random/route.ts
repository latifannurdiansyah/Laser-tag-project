import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

function generateRandomIndonesia(): { lat: number; lng: number; alt: number } {
  const lat = -11.0 + Math.random() * 8.0
  const lng = 95.0 + Math.random() * 25.0
  const alt = Math.floor(Math.random() * 100) + 10
  return { lat, lng, alt }
}

export async function POST() {
  try {
    const { lat, lng, alt } = generateRandomIndonesia()
    console.log('[API] Generating random coords:', lat, lng, alt)

    const irStatuses = ['-', '-', '-', '-', 'HIT: 0xABCD-0x12', 'HIT: 0x1234-0x34']
    const randomIrStatus = irStatuses[Math.floor(Math.random() * irStatuses.length)]

    const log = await prisma.gpsLog.create({
      data: {
        source: 'wifi',
        deviceId: 'RANDOM-GEN',
        latitude: lat,
        longitude: lng,
        altitude: alt,
        irStatus: randomIrStatus,
        battery: Math.floor(3600 + Math.random() * 600),
        satellites: Math.floor(5 + Math.random() * 8),
        rssi: Math.floor(-120 + Math.random() * 30),
        snr: parseFloat((Math.random() * 10 - 2).toFixed(1))
      }
    })

    console.log('[API] Random log created:', log.id)
    return NextResponse.json(log)
  } catch (error: any) {
    console.error('[API] Error generating random log:', error)
    return NextResponse.json({ error: error.message || 'Failed to generate random log' }, { status: 500 })
  }
}
