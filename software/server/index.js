import { initializeApp } from "https://www.gstatic.com/firebasejs/10.x.x/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/10.x.x/firebase-database.js";

// Ganti dengan konfigurasi dari Firebase Console Anda
const firebaseConfig = {
  apiKey: "AIzaSyBknkRTKdVwZInvip3SrzWDIdpefDRoIWI",
  databaseURL: "https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app",
  // ... rest of config
};

const app = initializeApp(firebaseConfig);
const db = getDatabase(app);
const tableBody = document.getElementById('table-body');

// Referensi ke 'gps_tracking' sesuai gambar database Anda
const gpsRef = ref(db, 'gps_tracking');

onValue(gpsRef, (snapshot) => {
  const data = snapshot.val();
  
  if (data) {
    // Jika data berbentuk objek tunggal (seperti di gambar pertama)
    tableBody.innerHTML = `
      <tr>
        <td>Device_01</td>
        <td>${data.latitude}</td>
        <td>${data.longitude}</td>
        <td>${new Date().toLocaleTimeString()}</td>
      </tr>
    `;
  } else {
    tableBody.innerHTML = '<tr><td colspan="4">Data tidak ditemukan.</td></tr>';
  }
});