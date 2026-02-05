'use client';

import { useEffect, useState } from 'react';

interface GpsLog {
  id: number;
  deviceId: string;
  latitude: number;
  longitude: number;
  createdAt: string;
}

export default function Home() {
  const [logs, setLogs] = useState<GpsLog[]>([]);
  const [loading, setLoading] = useState(true);
  const [lastUpdate, setLastUpdate] = useState<Date>(new Date());

  const fetchLogs = async () => {
    try {
      const res = await fetch('/api/track');
      const data = await res.json();
      if (data.success) {
        setLogs(data.data);
      }
    } catch (error) {
      console.error('Error fetching logs:', error);
    } finally {
      setLoading(false);
      setLastUpdate(new Date());
    }
  };

  useEffect(() => {
    fetchLogs();
    const interval = setInterval(fetchLogs, 5000);
    return () => clearInterval(interval);
  }, []);

  return (
    <main className="min-h-screen bg-gray-100 p-8">
      <div className="max-w-4xl mx-auto">
        <header className="mb-8">
          <h1 className="text-3xl font-bold text-gray-800">GPS Tracker Dashboard</h1>
          <p className="text-gray-600 mt-2">
            Test konektivitas GPS - Auto-refresh setiap 5 detik
          </p>
          <p className="text-sm text-gray-500 mt-1">
            Terakhir update: {lastUpdate.toLocaleTimeString()}
          </p>
        </header>

        <div className="bg-white rounded-lg shadow overflow-hidden">
          <div className="p-4 bg-blue-600 text-white flex justify-between items-center">
            <span className="font-semibold">GPS Logs</span>
            <button
              onClick={fetchLogs}
              className="px-4 py-1 bg-white text-blue-600 rounded text-sm font-medium hover:bg-blue-50"
            >
              Refresh Sekarang
            </button>
          </div>

          {loading && logs.length === 0 ? (
            <div className="p-8 text-center text-gray-500">
              Memuat data...
            </div>
          ) : logs.length === 0 ? (
            <div className="p-8 text-center text-gray-500">
              Belum ada data GPS. Kirim data dari ESP32 atau gunakan simulator.
            </div>
          ) : (
            <table className="w-full">
              <thead className="bg-gray-50">
                <tr>
                  <th className="px-4 py-3 text-left text-sm font-semibold text-gray-600">#</th>
                  <th className="px-4 py-3 text-left text-sm font-semibold text-gray-600">Device ID</th>
                  <th className="px-4 py-3 text-left text-sm font-semibold text-gray-600">Latitude</th>
                  <th className="px-4 py-3 text-left text-sm font-semibold text-gray-600">Longitude</th>
                  <th className="px-4 py-3 text-left text-sm font-semibold text-gray-600">Waktu</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-gray-200">
                {logs.map((log, index) => (
                  <tr key={log.id} className="hover:bg-gray-50">
                    <td className="px-4 py-3 text-sm text-gray-500">{index + 1}</td>
                    <td className="px-4 py-3 text-sm font-medium text-gray-800">{log.deviceId}</td>
                    <td className="px-4 py-3 text-sm text-gray-600">{log.latitude.toFixed(6)}</td>
                    <td className="px-4 py-3 text-sm text-gray-600">{log.longitude.toFixed(6)}</td>
                    <td className="px-4 py-3 text-sm text-gray-500">
                      {new Date(log.createdAt).toLocaleString()}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>

        <div className="mt-6 p-4 bg-blue-50 rounded-lg">
          <h3 className="font-semibold text-blue-800 mb-2">Informasi Endpoint</h3>
          <pre className="bg-white p-3 rounded text-sm overflow-x-auto">
{`POST /api/track
Body: { "id": "HELTEC-01", "lat": "-7.250000", "lng": "112.750000" }`}
          </pre>
        </div>
      </div>
    </main>
  );
}
