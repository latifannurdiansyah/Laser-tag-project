import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

// Enable CORS for HTTP requests from SIM900A
export async function OPTIONS(request: Request) {
  return new NextResponse(null, {
    status: 200,
    headers: {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'POST, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type',
    },
  })
}

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

    console.log('[API] Received GPS data from GPRS:', body)

    if (!id || lat === undefined || lng === undefined) {
      return NextResponse.json(
        { error: 'Missing required fields (id, lat, lng)' }, 
        { status: 400 }
      )
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
      const log = await prisma.gpsLog.create({
        data: {
          source: source || 'gprs',
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
      
      // Return with CORS headers for HTTP compatibility
      return new NextResponse(JSON.stringify(log), {
        status: 200,
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*',
        },
      })
    } catch (dbError: any) {
      console.log('[API] New schema not found, using old schema')
      const log = await prisma.gpsLog.create({
        data: {
          deviceId: id,
          latitude: parseFloat(lat),
          longitude: parseFloat(lng),
          irStatus: irStatusStr
        }
      })
      return new NextResponse(JSON.stringify(log), {
        status: 200,
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*',
        },
      })
    }
  } catch (error: any) {
    console.error('[API] Error creating track log:', error)
    return NextResponse.json(
      { error: error.message || 'Failed to create log' }, 
      { status: 500 }
    )
  }
}
