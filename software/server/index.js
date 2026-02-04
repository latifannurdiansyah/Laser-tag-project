import { db } from "./firebase.js";
import { ref, onValue, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

const tableBody = document.getElementById('table-body');
const locationsRef = ref(db, 'locations');

// Ambil 5 data terakhir agar tabel tidak terlalu panjang
const latestQuery = query(locationsRef, limitToLast(5));

onValue(latestQuery, (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // Kosongkan tabel sebelum diisi data baru
        tableBody.innerHTML = "";

        // Firebase mengembalikan objek, kita ubah ke array dan urutkan terbaru di atas
        const sortedData = Object.values(data).reverse();

        sortedData.forEach(item => {
            const row = document.createElement('tr');
            row.style.borderBottom = "1px solid #333";
            
            // Konversi timestamp milidetik ke waktu lokal
            const time = item.t ? new Date(item.t).toLocaleString('id-ID') : "N/A";

            row.innerHTML = `
                <td style="padding: 10px; color: #4fc3f7;">${item.device || 'Unknown'}</td>
                <td>${item.lat}</td>
                <td>${item.lng}</td>
                <td style="color: #888;">${time}</td>
            `;
            tableBody.appendChild(row);
        });
    }
}, (error) => {
    console.error("Error membaca Firebase:", error);
});