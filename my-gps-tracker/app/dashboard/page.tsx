'use client'

import { useEffect, useState } from 'react'

interface GpsLog {
  id: number
  deviceId: string
  latitude: number
  longitude: number
  createdAt: string
}

export default function Dashboard() {
  const [logs, setLogs] = useState<GpsLog[]>([])
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  const fetchLogs = async () => {
    try {
      const res = await fetch('/api/logs')
      if (!res.ok) {
        throw new Error(`HTTP ${res.status}`)
      }
      const data = await res.json()
      setLogs(data)
      setError(null)
    } catch (err: any) {
      console.error('Failed to fetch logs:', err)
      setError('Failed to connect to database. Please check Vercel logs.')
    }
  }

  useEffect(() => {
    fetchLogs()
    const interval = setInterval(fetchLogs, 2000)
    return () => clearInterval(interval)
  }, [])

  const handleRandom = async () => {
    setLoading(true)
    try {
      const res = await fetch('/api/random', { method: 'POST' })
      if (!res.ok) {
        throw new Error('Failed to generate random')
      }
      fetchLogs()
    } catch (err: any) {
      console.error('Generate random error:', err)
      alert('Error: ' + err.message)
    } finally {
      setLoading(false)
    }
  }

  const handleClear = async () => {
    if (!confirm('Delete all logs?')) return
    try {
      const res = await fetch('/api/logs', { method: 'DELETE' })
      if (!res.ok) {
        throw new Error('Failed to clear logs')
      }
      fetchLogs()
    } catch (err: any) {
      console.error('Clear error:', err)
      alert('Error: ' + err.message)
    }
  }

  return (
    <div style={{ minHeight: '100vh', background: '#0a0a0a', color: 'white', padding: '20px' }}>
      <div style={{ maxWidth: '1000px', margin: '0 auto' }}>
        <h1 style={{ fontSize: '28px', fontWeight: 'bold', marginBottom: '20px' }}>
          GPS Tracker Dashboard
        </h1>

        {error && (
          <div style={{ background: '#dc2626', padding: '12px', borderRadius: '6px', marginBottom: '20px' }}>
            {error}
          </div>
        )}

        <div style={{ display: 'flex', gap: '10px', marginBottom: '30px' }}>
          <button
            onClick={handleRandom}
            disabled={loading}
            style={{
              background: loading ? '#666' : '#16a34a',
              color: 'white',
              padding: '12px 24px',
              border: 'none',
              borderRadius: '6px',
              cursor: loading ? 'not-allowed' : 'pointer',
              fontSize: '14px'
            }}
          >
            {loading ? 'Loading...' : '+ Generate Random'}
          </button>
          <button
            onClick={handleClear}
            style={{
              background: '#dc2626',
              color: 'white',
              padding: '12px 24px',
              border: 'none',
              borderRadius: '6px',
              cursor: 'pointer',
              fontSize: '14px'
            }}
          >
            Clear All
          </button>
        </div>

        <div style={{ background: '#171717', borderRadius: '8px', overflow: 'hidden' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse' }}>
            <thead>
              <tr style={{ background: '#262626' }}>
                <th style={{ padding: '12px 16px', textAlign: 'left', fontWeight: '600', fontSize: '13px', color: '#a3a3a3', width: '60px' }}>ID</th>
                <th style={{ padding: '12px 16px', textAlign: 'left', fontWeight: '600', fontSize: '13px', color: '#a3a3a3', width: '140px' }}>DEVICE ID</th>
                <th style={{ padding: '12px 16px', textAlign: 'left', fontWeight: '600', fontSize: '13px', color: '#22c55e' }}>LATITUDE</th>
                <th style={{ padding: '12px 16px', textAlign: 'left', fontWeight: '600', fontSize: '13px', color: '#3b82f6' }}>LONGITUDE</th>
                <th style={{ padding: '12px 16px', textAlign: 'left', fontWeight: '600', fontSize: '13px', color: '#a3a3a3' }}>TIMESTAMP</th>
              </tr>
            </thead>
            <tbody>
              {logs.map((log, index) => (
                <tr
                  key={log.id}
                  style={{
                    background: index % 2 === 0 ? '#171717' : '#1f1f1f',
                    borderBottom: '1px solid #262626'
                  }}
                >
                  <td style={{ padding: '12px 16px', fontSize: '13px', color: '#737373' }}>{log.id}</td>
                  <td style={{ padding: '12px 16px', fontSize: '14px', color: '#e5e5e5' }}>{log.deviceId}</td>
                  <td style={{ padding: '12px 16px', fontSize: '14px', fontFamily: 'monospace', color: '#22c55e' }}>{log.latitude.toFixed(6)}</td>
                  <td style={{ padding: '12px 16px', fontSize: '14px', fontFamily: 'monospace', color: '#3b82f6' }}>{log.longitude.toFixed(6)}</td>
                  <td style={{ padding: '12px 16px', fontSize: '13px', color: '#737373' }}>
                    {new Date(log.createdAt).toLocaleString()}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>

          {logs.length === 0 && !error && (
            <div style={{ padding: '40px', textAlign: 'center', color: '#737373' }}>
              No GPS logs found
            </div>
          )}
        </div>

        <div style={{ marginTop: '20px', color: '#737373', fontSize: '13px' }}>
          Total logs: {logs.length}
        </div>
      </div>
    </div>
  )
}
