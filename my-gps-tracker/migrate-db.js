const { Client } = require('pg')

const client = new Client({
  connectionString: process.env.DATABASE_URL || 'postgresql://neondb_owner:npg_1M0oseQTBUyi@ep-lively-star-a1aqeuvs-pooler.ap-southeast-1.aws.neon.tech/neondb?sslmode=require&channel_binding=require'
})

async function migrate() {
  console.log('🔄 Starting database migration...')
  
  try {
    await client.connect()
    console.log('✅ Connected to database')

    const migrations = [
      `ALTER TABLE "gps_logs" ADD COLUMN "source" TEXT DEFAULT 'wifi'`,
      `ALTER TABLE "gps_logs" ADD COLUMN "altitude" REAL`,
      `ALTER TABLE "gps_logs" ADD COLUMN "irStatus" TEXT DEFAULT '-'`,
      `ALTER TABLE "gps_logs" ADD COLUMN "battery" INTEGER`,
      `ALTER TABLE "gps_logs" ADD COLUMN "satellites" INTEGER`,
      `ALTER TABLE "gps_logs" ADD COLUMN "rssi" INTEGER`,
      `ALTER TABLE "gps_logs" ADD COLUMN "snr" REAL`,
      `ALTER TABLE "gps_logs" ADD COLUMN "rawPayload" JSONB`
    ]

    for (const sql of migrations) {
      try {
        await client.query(sql)
        console.log(`✅ ${sql}`)
      } catch (err) {
        if (err.message.includes('already exists')) {
          console.log(`⏭️  Skipped (already exists): ${sql}`)
        } else {
          console.log(`❌ Error: ${err.message}`)
        }
      }
    }

    console.log('\n🎉 Migration completed!')
  } catch (err) {
    console.error('❌ Connection error:', err.message)
  } finally {
    await client.end()
  }
}

migrate()
