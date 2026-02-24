import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    console.log('[API] Received GPS data:', JSON.stringify(body))
    
    // Support field from firmware (deviceId, sats, hitStatus)
    // AND field from API (id, satellites, irStatus)
    const id = body.id || body.deviceId || null
    const lat = body.lat || body.latitude || null
    const lng = body.lng || body.longitude || null
    const alt = body.alt || body.altitude
    const source = body.source || 'wifi'
    const battery = body.battery || body.bat
    const satellites = body.satellites || body.sats
    const rssi = body.rssi
    const snr = body.snr
    const irStatus = body.irStatus || body.hitStatus || null
    const command = body.command || body.irCommand
    const address = body.address || body.irAddress

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
