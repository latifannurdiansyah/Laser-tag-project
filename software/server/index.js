// Menggunakan versi 10.8.0 yang stabil
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

// KONFIGURASI FIREBASE (WAJIB DIGANTI)
const firebaseConfig = {
  apiKey: "AIzaSyBknkRTKdVwZInvip3SrzWDIdpefDRoIWI",
  authDomain: "gps-log-a1d90.firebaseapp.com",
  databaseURL: "https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "gps-log-a1d90",
  storageBucket: "gps-log-a1d90.firebasestorage.app",
  messagingSenderId: "670421186715",
  appId: "1:670421186715:web:d01f0c518b09f300de2e3e",
  measurementId: "G-9VD5Y1MX6Z"
};

// Inisialisasi
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

// Elemen DOM
const tableBody = document.getElementById('table-body');
const treeContent = document.getElementById('tree-content');

// Referensi ke node 'gps_tracking'
const gpsRef = ref(db, 'gps_tracking');

// Mendengarkan perubahan data secara Realtime
onValue(gpsRef, (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // 1. Update Tampilan Tabel
        tableBody.innerHTML = `
            <tr>
                <td>ESP32_Device</td>
                <td>${data.latitude || '-'}</td>
                <td>${data.longitude || '-'}</td>
                <td>${data.altitude || '0'} m</td>
                <td>${data.satellites || '0'}</td>
            </tr>
        `;

        // 2. Update Tampilan Tree View (Mirip Firebase)
        treeContent.innerHTML = `
            <div class="node-item"><span class="key">altitude:</span> <span class="number highlight">${data.altitude}</span></div>
            <div class="node-item"><span class="key">latitude:</span> <span class="number">${data.latitude}</span></div>
            <div class="node-item"><span class="key">longitude:</span> <span class="number">${data.longitude}</span></div>
            <div class="node-item"><span class="key">satellites:</span> <span class="number highlight">${data.satellites}</span></div>
        `;
    } else {
        tableBody.innerHTML = '<tr><td colspan="5">Data kosong di Firebase.</td></tr>';
        treeContent.innerHTML = '<div class="node-item">Data tidak ditemukan.</div>';
    }
}, (error) => {
    console.error("Error membaca Firebase:", error);
    tableBody.innerHTML = `<tr><td colspan="5" style="color:red">Error: ${error.message}</td></tr>`;
});