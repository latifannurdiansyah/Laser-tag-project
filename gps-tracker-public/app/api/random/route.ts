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

    const log = await prisma.gpsLog.create({
      data: {
        deviceId: 'RANDOM-GEN',
        latitude: lat,
        longitude: lng
      }
    })

    return NextResponse.json(log)
  } catch (error) {
    return NextResponse.json({ error: 'Failed to generate random log' }, { status: 500 })
  }
}
