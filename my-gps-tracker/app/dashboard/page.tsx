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
  const [isMobile, setIsMobile] = useState(false)

  useEffect(() => {
    const checkMobile = () => {
      setIsMobile(window.innerWidth < 768)
    }
    checkMobile()
    window.addEventListener('resize', checkMobile)
    return () => window.removeEventListener('resize', checkMobile)
  }, [])

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

  const containerStyle: React.CSSProperties = {
    minHeight: '100vh',
    background: '#0a0a0a',
    color: 'white',
    padding: isMobile ? '12px' : '20px',
    width: '100%',
  }

  const contentStyle: React.CSSProperties = {
    maxWidth: isMobile ? '100%' : '1000px',
    margin: '0 auto',
  }

  const titleStyle: React.CSSProperties = {
    fontSize: isMobile ? '20px' : '28px',
    fontWeight: 'bold',
    marginBottom: isMobile ? '15px' : '20px',
  }

  const buttonGroupStyle: React.CSSProperties = {
    display: 'flex',
    gap: '10px',
    marginBottom: '20px',
    flexDirection: isMobile ? 'column' : 'row',
  }

  const buttonStyle: React.CSSProperties = {
    flex: isMobile ? 'none' : undefined,
    width: isMobile ? '100%' : undefined,
    background: '#16a34a',
    color: 'white',
    padding: isMobile ? '14px 20px' : '12px 24px',
    border: 'none',
    borderRadius: '8px',
    cursor: 'pointer',
    fontSize: isMobile ? '16px' : '14px',
    fontWeight: '600',
  }

  const clearButtonStyle: React.CSSProperties = {
    ...buttonStyle,
    background: '#dc2626',
  }

  const loadingButtonStyle: React.CSSProperties = {
    ...buttonStyle,
    background: '#666',
    cursor: 'not-allowed',
  }

  return (
    <div style={containerStyle}>
      <div style={contentStyle}>
        <h1 style={titleStyle}>
          GPS Tracker Dashboard
        </h1>

        {error && (
          <div style={{ 
            background: '#dc2626', 
            padding: '12px', 
            borderRadius: '6px', 
            marginBottom: '20px',
            fontSize: isMobile ? '14px' : '13px'
          }}>
            {error}
          </div>
        )}

        <div style={buttonGroupStyle}>
          <button
            onClick={handleRandom}
            disabled={loading}
            style={loading ? loadingButtonStyle : buttonStyle}
          >
            {loading ? 'Loading...' : '+ Generate Random'}
          </button>
          <button
            onClick={handleClear}
            style={loading ? loadingButtonStyle : clearButtonStyle}
          >
            Clear All
          </button>
        </div>

        {isMobile ? (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
            {logs.map((log, index) => (
              <div
                key={log.id}
                style={{
                  background: index % 2 === 0 ? '#171717' : '#1f1f1f',
                  borderRadius: '10px',
                  padding: '16px',
                  border: '1px solid #333',
                }}
              >
                <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '10px' }}>
                  <span style={{ fontSize: '14px', color: '#737373' }}>ID: {log.id}</span>
                  <span style={{ fontSize: '12px', color: '#a3a3a3' }}>
                    {new Date(log.createdAt).toLocaleString()}
                  </span>
                </div>
                <div style={{ fontSize: '14px', fontWeight: '600', color: '#e5e5e5', marginBottom: '8px' }}>
                  {log.deviceId}
                </div>
                <div style={{ display: 'flex', flexDirection: 'column', gap: '4px' }}>
                  <span style={{ fontSize: '14px', color: '#22c55e', fontFamily: 'monospace' }}>
                    Lat: {log.latitude.toFixed(6)}
                  </span>
                  <span style={{ fontSize: '14px', color: '#3b82f6', fontFamily: 'monospace' }}>
                    Lng: {log.longitude.toFixed(6)}
                  </span>
                </div>
              </div>
            ))}
            {logs.length === 0 && !error && (
              <div style={{ 
                padding: '40px', 
                textAlign: 'center', 
                color: '#737373',
                fontSize: isMobile ? '16px' : '14px'
              }}>
                No GPS logs found
              </div>
            )}
          </div>
        ) : (
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
        )}

        <div style={{ 
          marginTop: '20px', 
          color: '#737373', 
          fontSize: isMobile ? '14px' : '13px',
          textAlign: isMobile ? 'center' : 'left'
        }}>
          Total logs: {logs.length}
        </div>
      </div>
    </div>
  )
}
