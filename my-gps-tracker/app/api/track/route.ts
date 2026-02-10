import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const {
      source,
      id,
      lat,
      lng,
      alt,
      irStatus,
      battery,
      satellites,
      rssi,
      snr
    } = body

    console.log('[API] Received GPS data:', body)

    if (!id || lat === undefined || lng === undefined) {
      return NextResponse.json({ error: 'Missing required fields (id, lat, lng)' }, { status: 400 })
    }

    let irStatusStr = '-'
    if (irStatus !== undefined && irStatus !== null) {
      if (typeof irStatus === 'string') {
        irStatusStr = irStatus
      } else if (typeof irStatus === 'number') {
        irStatusStr = irStatus === 1 ? 'HIT' : '-'
      }
    }

    const log = await prisma.gpsLog.create({
      data: {
        source: source || 'wifi',
        deviceId: id,
        latitude: parseFloat(lat),
        longitude: parseFloat(lng),
        altitude: alt !== undefined ? parseFloat(alt) : null,
        irStatus: irStatusStr,
        battery: battery !== undefined ? parseInt(battery) : null,
        satellites: satellites !== undefined ? parseInt(satellites) : null,
        rssi: rssi !== undefined ? parseInt(rssi) : null,
        snr: snr !== undefined ? parseFloat(snr) : null,
        rawPayload: body
      }
    })

    console.log('[API] Track log created:', log.id)
    return NextResponse.json(log)
  } catch (error: any) {
    console.error('[API] Error creating track log:', error)
    return NextResponse.json({ error: error.message || 'Failed to create log' }, { status: 500 })
  }
}
