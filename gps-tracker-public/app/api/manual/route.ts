import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const { deviceId, lat, lng } = body

    if (!deviceId || lat === undefined || lng === undefined) {
      return NextResponse.json({ error: 'Missing required fields' }, { status: 400 })
    }

    const log = await prisma.gpsLog.create({
      data: {
        deviceId,
        latitude: parseFloat(lat),
        longitude: parseFloat(lng)
      }
    })

    return NextResponse.json(log)
  } catch (error) {
    return NextResponse.json({ error: 'Failed to create manual log' }, { status: 500 })
  }
}
