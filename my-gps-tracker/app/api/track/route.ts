import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    console.log('[API] Received GPS data:', JSON.stringify(body))
    
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

    console.log('[API] Parsed values - id:', id, 'lat:', lat, 'lng:', lng, 'alt:', alt)

    if (!id || lat === undefined || lng === undefined) {
      console.log('[API] Missing required fields - id:', id, 'lat:', lat, 'lng:', lng)
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

    const command = body.command
    const address = body.address
    if (command !== undefined && address !== undefined) {
      irStatusStr = `HIT: 0x${parseInt(address).toString(16).toUpperCase()}-0x${parseInt(command).toString(16).toUpperCase()}`
    }

    try {
      console.log('[API] Creating log with data:', { source, deviceId: id, lat, lng, alt })
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
      console.log('[API] Track log created successfully, id:', log.id)
      return NextResponse.json(log)
    } catch (dbError: any) {
      console.log('[API] New schema not found, using old schema, error:', dbError.message)
      const log = await prisma.gpsLog.create({
        data: {
          deviceId: id,
          latitude: parseFloat(lat),
          longitude: parseFloat(lng),
          irStatus: irStatusStr
        }
      })
      return NextResponse.json(log)
    }
  } catch (error: any) {
    console.error('[API] Error creating track log:', error)
    return NextResponse.json({ error: error.message || 'Failed to create log' }, { status: 500 })
  }
}
