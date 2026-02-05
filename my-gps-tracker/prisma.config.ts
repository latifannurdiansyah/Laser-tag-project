import { defineConfig } from '@prisma/config'
import path from 'path'
import { config } from 'dotenv'

// Memaksa dotenv membaca file .env.local
config({ path: path.resolve(process.cwd(), '.env.local') })

export default defineConfig({
  datasource: {
    url: process.env.POSTGRES_PRISMA_URL,
  },
})