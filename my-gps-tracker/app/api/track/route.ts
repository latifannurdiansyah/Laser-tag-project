import { PrismaClient } from '@prisma/client'
import { PrismaPg } from '@prisma/adapter-pg'
import { Pool } from 'pg'
import { NextResponse } from 'next/server'

const pool = new Pool({ connectionString: process.env.DATABASE_URL })
const adapter = new PrismaPg(pool)
const prisma = new PrismaClient({ adapter })

export async function POST(request: Request) {
  try {
    const body = await request.json()
    
    const newLog = await prisma.gpsLog.create({
      data: {
        deviceId: body.id || "HELTEC-01",
        latitude: parseFloat(body.lat),
        longitude: parseFloat(body.lng),
        //satellites: body.sats ? parseInt(body.sats) : null,
        //altitude: body.alt ? parseFloat(body.alt) : null,
      }
    })

    return NextResponse.json({ message: 'Sukses', id: newLog.id })
  } catch (error) {
    console.error(error)
    return NextResponse.json({ error: 'Gagal simpan' }, { status: 500 })
  }
}