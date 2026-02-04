import { db } from "./firebase.js"; // Pastikan path ke file firebase.js benar
import { ref, set, onValue } from "firebase/database";

// 1. Fungsi untuk Mengirim Data ke Firebase
function kirimDataTes() {
  const reference = ref(db, 'tes_koneksi/');
  set(reference, {
    pesan: "Halo dari Vercel!",
    waktu: new Date().toISOString(),
    status: "Berhasil Terhubung"
  }).then(() => {
    console.log("Data berhasil dikirim ke Firebase!");
    alert("Koneksi Sukses: Data terkirim!");
  }).catch((error) => {
    console.error("Gagal kirim data:", error);
  });
}

// 2. Fungsi untuk Membaca Data secara Realtime
function bacaDataTes() {
  const reference = ref(db, 'tes_koneksi/');
  onValue(reference, (snapshot) => {
    const data = snapshot.val();
    console.log("Data dari Firebase:", data);
    if (data) {
      document.body.innerHTML += `<h1>Pesan: ${data.pesan}</h1>`;
    }
  });
}

// Jalankan tes
kirimDataTes();
bacaDataTes();