// Menggunakan CDN agar bisa jalan di browser biasa
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, set, update, remove, push, onValue } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

const firebaseConfig = {
  // Langsung masukkan nilai dari gambar konfigurasi Anda
  apiKey: "AIzaSyBknkRTKdVwZInvip3SrzWDidpefDRoIWI",
  authDomain: "gps-log-a1d90.firebaseapp.com",
  databaseURL: "https://gps-log-a1d90-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "gps-log-a1d90",
  storageBucket: "gps-log-a1d90.firebasestorage.app",
  messagingSenderId: "670421186715",
  appId: "1:670421186715:web:d01f0c518b09f300de2e3e",
  measurementId: "G-9VD5Y1MX6Z"
};

const app = initializeApp(firebaseConfig);
export const db = getDatabase(app);