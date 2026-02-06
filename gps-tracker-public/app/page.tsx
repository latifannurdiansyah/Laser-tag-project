"use client";

import { useState, useEffect } from "react";

interface GpsData {
  id: number;
  deviceId: string;
  latitude: number;
  longitude: number;
  altitude: number | null;
  satellites: number | null;
  createdAt: string;
}

interface ApiResponse {
  success: boolean;
  data: GpsData;
  pagination?: {
    total: number;
    limit: number;
    offset: number;
    hasMore: boolean;
  };
}

const API_BASE = "/api/track";

export default function Home() {
  const [latestData, setLatestData] = useState<GpsData | null>(null);
  const [historyData, setHistoryData] = useState<GpsData[]>([]);
  const [loading, setLoading] = useState(true);
  const [selectedDevice, setSelectedDevice] = useState<string>("");
  const [lastUpdate, setLastUpdate] = useState<Date>(new Date());
  const [error, setError] = useState<string | null>(null);
  const [devices, setDevices] = useState<string[]>(["Heltec-1", "Heltec-2", "Heltec-3"]);

  const fetchData = async () => {
    try {
      setError(null);
      const deviceParam = selectedDevice ? `?deviceId=${selectedDevice}` : "";

      const [latestRes, historyRes] = await Promise.all([
        fetch(`${API_BASE}/latest${deviceParam}`),
        fetch(`${API_BASE}/history${deviceParam}&limit=20`),
      ]);

      if (!latestRes.ok) throw new Error("Failed to fetch latest data");
      const latestJson: ApiResponse = await latestRes.json();
      setLatestData(latestJson.data);
      setLastUpdate(new Date());

      if (!historyRes.ok) throw new Error("Failed to fetch history");
      const historyJson: ApiResponse = await historyRes.json();
      setHistoryData(historyJson.data);

      const allDevices = new Set([
        ...devices,
        ...historyJson.data.map((d) => d.deviceId),
      ]);
      setDevices(Array.from(allDevices));
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unknown error");
      console.error("[ERROR] Fetch failed:", err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 5000);
    return () => clearInterval(interval);
  }, [selectedDevice]);

  const formatTime = (isoString: string) => {
    const date = new Date(isoString);
    return date.toLocaleTimeString("id-ID", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
  };

  const formatDate = (isoString: string) => {
    const date = new Date(isoString);
    return date.toLocaleDateString("id-ID", {
      day: "2-digit",
      month: "short",
      year: "numeric",
    });
  };

  const getStatusColor = () => {
    if (!latestData) return "status-offline";
    const diff = (Date.now() - new Date(latestData.createdAt).getTime()) / 1000;
    if (diff > 15) return "status-offline";
    if (diff > 10) return "status-updating";
    return "status-online";
  };

  const timeSinceUpdate = latestData
    ? Math.floor((Date.now() - new Date(latestData.createdAt).getTime()) / 1000)
    : 0;

  return (
    <main className="min-h-screen p-4 md:p-8">
      <div className="max-w-6xl mx-auto">
        <header className="mb-8">
          <h1 className="text-3xl font-bold text-white mb-2">GPS Tracker Dashboard</h1>
          <p className="text-gray-400">Live tracking untuk Heltec devices</p>
        </header>

        <div className="mb-6">
          <label className="block text-sm font-medium text-gray-300 mb-2">
            Select Device
          </label>
          <select
            value={selectedDevice}
            onChange={(e) => setSelectedDevice(e.target.value)}
            className="w-full md:w-64 px-4 py-2 bg-gray-800 border border-gray-700 rounded-lg text-white focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
          >
            <option value="">All Devices</option>
            {devices.map((device) => (
              <option key={device} value={device}>
                {device}
              </option>
            ))}
          </select>
        </div>

        {error && (
          <div className="mb-6 p-4 bg-red-900/20 border border-red-800 rounded-lg text-red-400">
            Error: {error}
          </div>
        )}

        {loading && !latestData ? (
          <div className="flex items-center justify-center h-64">
            <div className="text-gray-400">Loading...</div>
          </div>
        ) : (
          <>
            <div className="grid md:grid-cols-2 gap-6 mb-8">
              <div className="card">
                <div className="flex items-center justify-between mb-4">
                  <h2 className="text-lg font-semibold text-white">Status</h2>
                  <div className={`flex items-center gap-2 ${getStatusColor()}`}>
                    <div className={`w-3 h-3 rounded-full ${timeSinceUpdate < 10 ? "bg-green-500" : timeSinceUpdate < 15 ? "bg-yellow-500" : "bg-red-500"} ${timeSinceUpdate < 10 ? "pulse-animation" : ""}`}></div>
                    <span className="text-sm font-medium">
                      {latestData ? "Online" : "Offline"}
                    </span>
                  </div>
                </div>
                <div className="space-y-2 text-sm">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Last Update:</span>
                    <span className="text-white">
                      {latestData ? formatTime(latestData.createdAt) : "-"}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Device:</span>
                    <span className="text-white">{latestData?.deviceId || "-"}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Time Since Update:</span>
                    <span className="text-white">{timeSinceUpdate}s ago</span>
                  </div>
                </div>
              </div>

              <div className="card">
                <h2 className="text-lg font-semibold text-white mb-4">Current Position</h2>
                {latestData ? (
                  <div className="space-y-3">
                    <div>
                      <span className="text-gray-400 text-sm">Latitude</span>
                      <p className="text-2xl font-mono text-white">{latestData.latitude.toFixed(6)}</p>
                    </div>
                    <div>
                      <span className="text-gray-400 text-sm">Longitude</span>
                      <p className="text-2xl font-mono text-white">{latestData.longitude.toFixed(6)}</p>
                    </div>
                    <div className="flex gap-6 pt-2">
                      <div>
                        <span className="text-gray-400 text-xs">Altitude</span>
                        <p className="text-white font-medium">{latestData.altitude?.toFixed(1) || "-"} m</p>
                      </div>
                      <div>
                        <span className="text-gray-400 text-xs">Satellites</span>
                        <p className="text-white font-medium">{latestData.satellites || "-"}</p>
                      </div>
                    </div>
                  </div>
                ) : (
                  <p className="text-gray-400">Waiting for GPS data...</p>
                )}
              </div>
            </div>

            <div className="card">
              <div className="flex items-center justify-between mb-4">
                <h2 className="text-lg font-semibold text-white">GPS Logs</h2>
                <button
                  onClick={fetchData}
                  className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white text-sm font-medium rounded-lg transition-colors"
                >
                  Refresh
                </button>
              </div>
              <div className="overflow-x-auto">
                <table className="w-full text-sm">
                  <thead>
                    <tr className="border-b border-gray-700">
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Time</th>
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Device</th>
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Latitude</th>
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Longitude</th>
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Alt</th>
                      <th className="text-left py-3 px-4 text-gray-400 font-medium">Sats</th>
                    </tr>
                  </thead>
                  <tbody>
                    {historyData.length === 0 ? (
                      <tr>
                        <td colSpan={6} className="py-8 text-center text-gray-400">
                          No GPS data available
                        </td>
                      </tr>
                    ) : (
                      historyData.map((data) => (
                        <tr key={data.id} className="border-b border-gray-800 hover:bg-gray-800/50">
                          <td className="py-3 px-4 text-white font-mono text-xs">
                            {formatDate(data.createdAt)} {formatTime(data.createdAt)}
                          </td>
                          <td className="py-3 px-4 text-indigo-400">{data.deviceId}</td>
                          <td className="py-3 px-4 text-white font-mono">{data.latitude.toFixed(6)}</td>
                          <td className="py-3 px-4 text-white font-mono">{data.longitude.toFixed(6)}</td>
                          <td className="py-3 px-4 text-gray-300">{data.altitude?.toFixed(1) || "-"}</td>
                          <td className="py-3 px-4 text-gray-300">{data.satellites || "-"}</td>
                        </tr>
                      ))
                    )}
                  </tbody>
                </table>
              </div>
            </div>
          </>
        )}

        <footer className="mt-8 text-center text-gray-500 text-sm">
          <p>Auto-refresh setiap 5 detik • Data tersimpan di NeonDB PostgreSQL</p>
        </footer>
      </div>
    </main>
  );
}