'use client'

import { useEffect, useState } from 'react'

interface GpsLog {
  id: number
  source?: string | null
  deviceId: string
  latitude: number
  longitude: number
  altitude?: number | null
  irStatus?: string | null
  battery?: number | null
  satellites?: number | null
  rssi?: number | null
  snr?: number | null
  createdAt: string
}

export default function Dashboard() {
  const [logs, setLogs] = useState<GpsLog[]>([])
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)
  const [isMobile, setIsMobile] = useState(false)

  useEffect(() => {
    const checkMobile = () => {
      setIsMobile(window.innerWidth < 1024)
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

  const getBatteryColor = (mV: number | null) => {
    if (mV === null) return '#737373'
    if (mV > 4200) return '#22c55e'
    if (mV > 3800) return '#eab308'
    return '#ef4444'
  }

  const getBatteryIcon = (mV: number | null) => {
    if (mV === null) return '🔋'
    if (mV > 4200) return '🟢'
    if (mV > 3800) return '🟡'
    if (mV > 3400) return '🟠'
    return '🔴'
  }

  const getSignalColor = (rssi: number | null) => {
    if (rssi === null) return '#737373'
    if (rssi >= -100) return '#22c55e'
    if (rssi >= -110) return '#eab308'
    return '#ef4444'
  }

  const containerStyle: React.CSSProperties = {
    minHeight: '100vh',
    background: '#0a0a0a',
    color: 'white',
    padding: isMobile ? '12px' : '20px',
    width: '100%',
  }

  const contentStyle: React.CSSProperties = {
    maxWidth: isMobile ? '100%' : '1400px',
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

  const sourceBadgeStyle = (source: string): React.CSSProperties => ({
    padding: '2px 8px',
    borderRadius: '4px',
    fontSize: '11px',
    fontWeight: 600,
    textTransform: 'uppercase',
    background: source === 'ttn' ? '#7c3aed' : '#2563eb',
    color: 'white',
  })

  const isIrHit = (status: string | null) => {
    if (!status || status === '-') return false
    return status.toUpperCase().startsWith('HIT')
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
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
                  <span style={{ fontSize: '12px', color: '#737373' }}>#{log.id}</span>
                  <span style={sourceBadgeStyle(log.source || 'wifi')}>{log.source || 'wifi'}</span>
                </div>

                <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '8px' }}>
                  <span style={{ fontSize: '14px', fontWeight: '600', color: '#e5e5e5' }}>
                    {log.deviceId}
                  </span>
                  <span style={{ fontSize: '11px', color: '#a3a3a3' }}>
                    {new Date(log.createdAt).toLocaleString()}
                  </span>
                </div>

                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px', marginBottom: '12px' }}>
                  <div style={{ background: '#0a0a0a', padding: '10px', borderRadius: '6px' }}>
                    <div style={{ fontSize: '10px', color: '#737373', marginBottom: '2px' }}>LATITUDE</div>
                    <div style={{ fontSize: '13px', fontFamily: 'monospace', color: '#22c55e' }}>
                      {log.latitude.toFixed(6)}
                    </div>
                  </div>
                  <div style={{ background: '#0a0a0a', padding: '10px', borderRadius: '6px' }}>
                    <div style={{ fontSize: '10px', color: '#737373', marginBottom: '2px' }}>LONGITUDE</div>
                    <div style={{ fontSize: '13px', fontFamily: 'monospace', color: '#3b82f6' }}>
                      {log.longitude.toFixed(6)}
                    </div>
                  </div>
                </div>

                <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '8px', marginBottom: '12px' }}>
                  <div style={{ background: '#0a0a0a', padding: '8px', borderRadius: '6px', textAlign: 'center' }}>
                    <div style={{ fontSize: '9px', color: '#737373' }}>ALT</div>
                    <div style={{ fontSize: '12px', color: '#a3a3a3' }}>
                      {log.altitude != null ? `${log.altitude.toFixed(1)}m` : '-'}
                    </div>
                  </div>
                  <div style={{ background: '#0a0a0a', padding: '8px', borderRadius: '6px', textAlign: 'center' }}>
                    <div style={{ fontSize: '9px', color: '#737373' }}>SAT</div>
                    <div style={{ fontSize: '12px', color: '#a3a3a3' }}>
                      {log.satellites != null ? log.satellites : '-'}
                    </div>
                  </div>
                  <div style={{ background: '#0a0a0a', padding: '8px', borderRadius: '6px', textAlign: 'center' }}>
                    <div style={{ fontSize: '9px', color: '#737373' }}>BATT</div>
                    <div style={{ fontSize: '12px', color: getBatteryColor(log.battery) }}>
                      {log.battery != null ? `${(log.battery / 1000).toFixed(2)}V` : '-'}
                    </div>
                  </div>
                </div>

                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px', marginBottom: '12px' }}>
                  <div style={{ background: '#0a0a0a', padding: '8px', borderRadius: '6px', textAlign: 'center' }}>
                    <div style={{ fontSize: '9px', color: '#737373' }}>RSSI</div>
                    <div style={{ fontSize: '12px', color: getSignalColor(log.rssi) }}>
                      {log.rssi != null ? `${log.rssi} dBm` : '-'}
                    </div>
                  </div>
                  <div style={{ background: '#0a0a0a', padding: '8px', borderRadius: '6px', textAlign: 'center' }}>
                    <div style={{ fontSize: '9px', color: '#737373' }}>SNR</div>
                    <div style={{ fontSize: '12px', color: log.snr != null && log.snr >= 0 ? '#22c55e' : '#737373' }}>
                      {log.snr != null ? `${log.snr.toFixed(1)} dB` : '-'}
                    </div>
                  </div>
                </div>

                <div style={{
                  background: isIrHit(log.irStatus || '-') ? '#7f1d1d' : '#0a0a0a',
                  padding: '10px',
                  borderRadius: '6px',
                  textAlign: 'center',
                  border: isIrHit(log.irStatus || '-') ? '1px solid #ef4444' : '1px solid #262626'
                }}>
                  <span style={{
                    fontSize: '12px',
                    fontWeight: '600',
                    color: isIrHit(log.irStatus || '-') ? '#ef4444' : '#737373'
                  }}>
                    STATUS: {log.irStatus || '-'}
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
            <div style={{ overflowX: 'auto' }}>
              <table style={{ width: '100%', borderCollapse: 'collapse', minWidth: '1000px' }}>
                <thead>
                  <tr style={{ background: '#262626' }}>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '50px' }}>ID</th>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '80px' }}>SRC</th>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '120px' }}>DEVICE</th>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#22c55e' }}>LATITUDE</th>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#3b82f6' }}>LONGITUDE</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '80px' }}>ALT (m)</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '60px' }}>SAT</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '90px' }}>BATT</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '90px' }}>RSSI</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '80px' }}>SNR</th>
                    <th style={{ padding: '10px 12px', textAlign: 'center', fontWeight: 600, fontSize: '12px', color: '#ef4444', width: '140px' }}>STATUS</th>
                    <th style={{ padding: '10px 12px', textAlign: 'left', fontWeight: 600, fontSize: '12px', color: '#a3a3a3', width: '150px' }}>TIMESTAMP</th>
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
                      <td style={{ padding: '10px 12px', fontSize: '12px', color: '#737373' }}>{log.id}</td>
                      <td style={{ padding: '10px 12px' }}>
                        <span style={sourceBadgeStyle(log.source || 'wifi')}>{log.source || 'wifi'}</span>
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', color: '#e5e5e5' }}>{log.deviceId}</td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', fontFamily: 'monospace', color: '#22c55e' }}>{log.latitude.toFixed(6)}</td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', fontFamily: 'monospace', color: '#3b82f6' }}>{log.longitude.toFixed(6)}</td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', textAlign: 'center', color: '#a3a3a3' }}>
                        {log.altitude != null ? log.altitude.toFixed(1) : '-'}
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', textAlign: 'center', color: '#a3a3a3' }}>
                        {log.satellites != null ? log.satellites : '-'}
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', textAlign: 'center', color: getBatteryColor(log.battery) }}>
                        {log.battery != null ? `${(log.battery / 1000).toFixed(2)}V` : '-'}
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', textAlign: 'center', color: getSignalColor(log.rssi) }}>
                        {log.rssi != null ? `${log.rssi} dBm` : '-'}
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '13px', textAlign: 'center', color: log.snr != null && log.snr >= 0 ? '#22c55e' : '#737373' }}>
                        {log.snr != null ? `${log.snr.toFixed(1)} dB` : '-'}
                      </td>
                      <td style={{
                        padding: '10px 12px',
                        fontSize: '12px',
                        fontWeight: 600,
                        textAlign: 'center',
                        color: isIrHit(log.irStatus || '-') ? '#ef4444' : '#737373',
                        background: isIrHit(log.irStatus || '-') ? 'rgba(239, 68, 68, 0.1)' : 'transparent'
                      }}>
                        {log.irStatus || '-'}
                      </td>
                      <td style={{ padding: '10px 12px', fontSize: '12px', color: '#737373' }}>
                        {new Date(log.createdAt).toLocaleString()}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
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
          textAlign: isMobile ? 'center' : 'left',
          display: 'flex',
          gap: '20px',
          flexWrap: 'wrap'
        }}>
          <span>Total logs: {logs.length}</span>
          <span>Source: WiFi = WiFi HTTP, TTN = LoRaWAN</span>
          <span>Status HIT = Player ditembak (merah)</span>
        </div>
      </div>
    </div>
  )
}
