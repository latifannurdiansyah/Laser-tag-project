import { NextResponse } from 'next/server'
import { prisma } from '@/lib/prisma'

export async function GET() {
  try {
    console.log('[API] Fetching logs from database...')
    const logs = await prisma.gpsLog.findMany({
      orderBy: { createdAt: 'desc' }
    })
    console.log('[API] Logs found:', logs.length)
    return NextResponse.json(logs)
  } catch (error: any) {
    console.error('[API] Error fetching logs:', error)
    return NextResponse.json({ error: error.message || 'Unknown error' }, { status: 500 })
  }
}

export async function DELETE() {
  try {
    await prisma.gpsLog.deleteMany()
    console.log('[API] All logs deleted')
    return NextResponse.json({ message: 'All logs deleted' })
  } catch (error: any) {
    console.error('[API] Error deleting logs:', error)
    return NextResponse.json({ error: error.message || 'Unknown error' }, { status: 500 })
  }
}
