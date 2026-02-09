import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const { id, lat, lng, irStatus } = body

    console.log('[API] Received GPS data:', body)

    if (!id || lat === undefined || lng === undefined) {
      return NextResponse.json({ error: 'Missing required fields' }, { status: 400 })
    }

    const log = await prisma.gpsLog.create({
      data: {
        deviceId: id,
        latitude: parseFloat(lat),
        longitude: parseFloat(lng),
        irStatus: irStatus || '-'
      }
    })

    console.log('[API] Track log created:', log.id)
    return NextResponse.json(log)
  } catch (error: any) {
    console.error('[API] Error creating track log:', error)
    return NextResponse.json({ error: error.message || 'Failed to create log' }, { status: 500 })
  }
}
