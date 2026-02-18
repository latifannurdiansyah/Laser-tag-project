import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    console.log('[TTN] Received webhook:', JSON.stringify(body, null, 2))

    const { end_device_ids, uplink_message } = body

    if (!end_device_ids || !uplink_message) {
      console.log('[TTN] Invalid payload format')
      return NextResponse.json({ error: 'Invalid payload format' }, { status: 400 })
    }

    const deviceId = end_device_ids.device_id || end_device_ids.dev_eui || 'UNKNOWN'
    const receivedAt = uplink_message.received_at || new Date().toISOString()

    let parsedData = uplink_message.decoded_payload || uplink_message.payload

    if (!parsedData || typeof parsedData !== 'object') {
      console.log('[TTN] No decoded payload found')
      return NextResponse.json({ error: 'No decoded payload' }, { status: 400 })
    }

    const {
      deviceId: payloadDeviceId,
      lat,
      lng,
      alt,
      irStatus,
      battery,
      satellites,
      rssi,
      snr
    } = parsedData

    const finalDeviceId = payloadDeviceId || deviceId

    let irStatusStr = '-'
    if (irStatus !== undefined && irStatus !== null) {
      if (typeof irStatus === 'string') {
        irStatusStr = irStatus
      } else if (typeof irStatus === 'number') {
        irStatusStr = irStatus === 1 ? 'HIT' : '-'
      } else if (typeof irStatus === 'object' && irStatus.address !== undefined) {
        irStatusStr = `HIT: 0x${irStatus.address.toString(16).toUpperCase()}-0x${irStatus.command.toString(16).toUpperCase()}`
      }
    }

    const command = parsedData.command
    const address = parsedData.address
    if (command !== undefined && address !== undefined) {
      irStatusStr = `HIT: 0x${parseInt(address).toString(16).toUpperCase()}-0x${parseInt(command).toString(16).toUpperCase()}`
    }

    const log = await prisma.gpsLog.create({
      data: {
        source: 'ttn',
        deviceId: finalDeviceId,
        latitude: lat || 0,
        longitude: lng || 0,
        altitude: alt !== undefined ? parseFloat(alt) : null,
        irStatus: irStatusStr,
        battery: battery !== undefined ? parseInt(battery) : null,
        satellites: satellites !== undefined ? parseInt(satellites) : null,
        rssi: rssi !== undefined ? parseInt(rssi) : null,
        snr: snr !== undefined ? parseFloat(snr) : null,
        rawPayload: {
          end_device_ids: end_device_ids,
          uplink_message: {
            received_at: receivedAt,
            f_cnt: uplink_message.f_cnt,
            f_port: uplink_message.f_port,
            rssi: uplink_message.rssi,
            snr: uplink_message.snr,
            decoded_payload: parsedData
          }
        }
      }
    })

    console.log('[TTN] Log created:', log.id)
    return NextResponse.json(log)
  } catch (error: any) {
    console.error('[TTN] Error:', error)
    return NextResponse.json({ error: error.message || 'Failed to process TTN webhook' }, { status: 500 })
  }
}
