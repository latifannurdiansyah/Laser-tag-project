'use client'

import { useState, useEffect } from 'react'
import { useRouter } from 'next/navigation'

export default function Home() {
  const router = useRouter()
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    const isLoggedIn = localStorage.getItem('lasertag_logged_in')
    if (isLoggedIn === 'true') {
      router.push('/dashboard')
    }
  }, [router])

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault()
    setError('')
    setLoading(true)

    await new Promise(resolve => setTimeout(resolve, 500))

    if (email === 'admin' && password === '123') {
      localStorage.setItem('lasertag_logged_in', 'true')
      localStorage.setItem('lasertag_user', email)
      router.push('/dashboard')
    } else {
      setError('Invalid credentials')
      setLoading(false)
    }
  }

  return (
    <div style={{
      minHeight: '100vh',
      background: '#0a0a0a',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      padding: '20px',
      fontFamily: '"Courier New", monospace',
    }}>
      <div style={{
        width: '100%',
        maxWidth: '400px',
        background: '#111111',
        borderRadius: '16px',
        padding: '40px 30px',
        border: '1px solid #22c55e',
        boxShadow: '0 0 30px rgba(34, 197, 94, 0.15)',
      }}>
        <div style={{ textAlign: 'center', marginBottom: '30px' }}>
          <div style={{ 
            fontSize: '48px', 
            marginBottom: '10px',
            filter: 'drop-shadow(0 0 10px rgba(34, 197, 94, 0.5))'
          }}>
            🎯
          </div>
          <h1 style={{
            fontSize: '24px',
            fontWeight: 'bold',
            color: '#22c55e',
            letterSpacing: '4px',
            marginBottom: '5px',
            textTransform: 'uppercase',
          }}>
            Lasertag
          </h1>
          <div style={{
            fontSize: '12px',
            color: '#666',
            letterSpacing: '2px',
          }}>
            SYSTEM ACCESS
          </div>
        </div>

        <form onSubmit={handleLogin}>
          <div style={{ marginBottom: '20px' }}>
            <label style={{
              display: 'block',
              fontSize: '11px',
              color: '#888',
              marginBottom: '8px',
              letterSpacing: '1px',
              textTransform: 'uppercase',
            }}>
              Username
            </label>
            <input
              type="text"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="admin"
              style={{
                width: '100%',
                padding: '14px 16px',
                background: '#0a0a0a',
                border: '1px solid #333',
                borderRadius: '8px',
                color: '#fff',
                fontSize: '14px',
                fontFamily: 'inherit',
                outline: 'none',
                transition: 'border-color 0.2s',
                boxSizing: 'border-box',
              }}
              onFocus={(e) => e.target.style.borderColor = '#22c55e'}
              onBlur={(e) => e.target.style.borderColor = '#333'}
            />
          </div>

          <div style={{ marginBottom: '25px' }}>
            <label style={{
              display: 'block',
              fontSize: '11px',
              color: '#888',
              marginBottom: '8px',
              letterSpacing: '1px',
              textTransform: 'uppercase',
            }}>
              Password
            </label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="••••••••"
              style={{
                width: '100%',
                padding: '14px 16px',
                background: '#0a0a0a',
                border: '1px solid #333',
                borderRadius: '8px',
                color: '#fff',
                fontSize: '14px',
                fontFamily: 'inherit',
                outline: 'none',
                transition: 'border-color 0.2s',
                boxSizing: 'border-box',
              }}
              onFocus={(e) => e.target.style.borderColor = '#22c55e'}
              onBlur={(e) => e.target.style.borderColor = '#333'}
            />
          </div>

          {error && (
            <div style={{
              background: 'rgba(239, 68, 68, 0.1)',
              border: '1px solid #ef4444',
              borderRadius: '8px',
              padding: '12px',
              marginBottom: '20px',
              color: '#ef4444',
              fontSize: '13px',
              textAlign: 'center',
            }}>
              {error}
            </div>
          )}

          <button
            type="submit"
            disabled={loading}
            style={{
              width: '100%',
              padding: '14px',
              background: loading ? '#333' : '#22c55e',
              border: 'none',
              borderRadius: '8px',
              color: loading ? '#666' : '#0a0a0a',
              fontSize: '14px',
              fontWeight: 'bold',
              fontFamily: 'inherit',
              letterSpacing: '2px',
              textTransform: 'uppercase',
              cursor: loading ? 'not-allowed' : 'pointer',
              transition: 'all 0.2s',
              boxShadow: loading ? 'none' : '0 0 20px rgba(34, 197, 94, 0.3)',
            }}
            onMouseOver={(e) => {
              if (!loading) {
                e.currentTarget.style.background = '#16a34a'
                e.currentTarget.style.boxShadow = '0 0 30px rgba(34, 197, 94, 0.5)'
              }
            }}
            onMouseOut={(e) => {
              if (!loading) {
                e.currentTarget.style.background = '#22c55e'
                e.currentTarget.style.boxShadow = '0 0 20px rgba(34, 197, 94, 0.3)'
              }
            }}
          >
            {loading ? 'Accessing...' : 'Login'}
          </button>
        </form>

        <div style={{
          marginTop: '25px',
          textAlign: 'center',
          fontSize: '11px',
          color: '#444',
        }}>
          <div style={{ marginBottom: '5px' }}>🎯 TARGETING... READY</div>
          <div style={{ color: '#333' }}>v1.0.0</div>
        </div>
      </div>
    </div>
  )
}
