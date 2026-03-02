import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    console.log('[API] Received GPS data:', JSON.stringify(body))
    
    // Support field from firmware (deviceId, sats, hitStatus)
    // AND field from API (id, satellites, irStatus)
    // Use ?? (nullish coalescing) instead of || to handle 0 values
    const id = body.id ?? body.deviceId ?? null
    const lat = body.lat ?? body.latitude ?? null
    const lng = body.lng ?? body.longitude ?? null
    const alt = body.alt ?? body.altitude
    const source = body.source ?? 'wifi'
    const battery = body.battery ?? body.bat
    const satellites = body.satellites ?? body.sats
    const rssi = body.rssi
    const snr = body.snr
    const irStatus = body.irStatus ?? body.hitStatus ?? null
    const command = body.command ?? body.irCommand
    const address = body.address ?? body.irAddress

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

    // Only set IR hit status if both command and address are non-zero
    if (command !== undefined && address !== undefined && command !== 0 && address !== 0) {
      irStatusStr = `HIT: 0x${parseInt(address).toString(16).toUpperCase()}-0x${parseInt(command).toString(16).toUpperCase()}`
    }

    try {
      console.log('[API] Creating log with data:', { source, deviceId: id, lat, lng, alt })
      const log = await prisma.gpsLog.create({
        data: {
          source: source ?? 'wifi',
          deviceId: id,
          latitude: lat ?? 0,
          longitude: lng ?? 0,
          altitude: alt != null ? parseFloat(alt) : null,
          irStatus: irStatusStr,
          battery: battery != null ? parseInt(battery) : null,
          satellites: satellites != null ? parseInt(satellites) : null,
          rssi: rssi != null ? parseInt(rssi) : null,
          snr: snr != null ? parseFloat(snr) : null,
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
          latitude: lat ?? 0,
          longitude: lng ?? 0,
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
