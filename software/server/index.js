import { db } from "./firebase.js";
import { ref, onValue, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

const locationsRef = ref(db, 'locations');
// ... sisa kode onValue Anda ...