import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, onValue, set, update, remove } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

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

const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

const tableBody = document.getElementById('table-body');
const treeContent = document.getElementById('tree-content');
const deviceCount = document.getElementById('deviceCount');
const lastUpdate = document.getElementById('lastUpdate');
const connectionStatus = document.getElementById('connectionStatus');
const statusText = document.getElementById('statusText');

let currentData = null;
let isConnected = false;

const gpsRef = ref(db, 'gps_tracking');

onValue(gpsRef, (snapshot) => {
    isConnected = true;
    connectionStatus.classList.remove('disconnected');
    statusText.textContent = 'Connected';
    
    const data = snapshot.val();
    currentData = data;
    const now = new Date().toLocaleTimeString();
    lastUpdate.textContent = now;
    
    console.log('[INFO] Firebase data updated at', now);
    console.log('[INFO] Raw snapshot:', data);
    
    if (DevTools) {
        DevTools.updateJsonPreview(data);
    }
    
    if (data) {
        const devices = Object.keys(data);
        console.log('[INFO] Found', devices.length, 'device(s):', devices);
        deviceCount.textContent = `${devices.length} Device${devices.length !== 1 ? 's' : ''}`;
        
        let tableHTML = '';
        let treeHTML = '';
        
        if (devices.length === 1 && devices[0] === 'latitude') {
            const singleDevice = data;
            tableBody.innerHTML = `
                <tr>
                    <td>
                        <div class="device-id">
                            <div class="device-icon">
                                <i class="fas fa-microchip"></i>
                            </div>
                            <div>
                                <div class="device-name">ESP32 Device</div>
                                <div class="device-id-text">gps_tracking</div>
                            </div>
                        </div>
                    </td>
                    <td>
                        <div class="coordinate">
                            <span class="coord-value">${singleDevice.latitude || '-'}</span>
                            <span class="coord-label">Latitude</span>
                        </div>
                    </td>
                    <td>
                        <div class="coordinate">
                            <span class="coord-value">${singleDevice.longitude || '-'}</span>
                            <span class="coord-label">Longitude</span>
                        </div>
                    </td>
                    <td>
                        <span class="stat-badge altitude">
                            <i class="fas fa-mountain"></i>
                            ${singleDevice.altitude || '0'} m
                        </span>
                    </td>
                    <td>
                        <span class="stat-badge satellites">
                            <i class="fas fa-satellite"></i>
                            ${singleDevice.satellites || '0'}
                        </span>
                    </td>
                </tr>
            `;
            
            treeHTML = `
                <div class="tree-item">
                    <i class="fas fa-chevron-right tree-toggle"></i>
                    <span class="tree-key">altitude:</span>
                    <span class="tree-value number">${singleDevice.altitude}</span>
                </div>
                <div class="tree-item">
                    <i class="fas fa-chevron-right tree-toggle"></i>
                    <span class="tree-key">latitude:</span>
                    <span class="tree-value number">${singleDevice.latitude}</span>
                </div>
                <div class="tree-item">
                    <i class="fas fa-chevron-right tree-toggle"></i>
                    <span class="tree-key">longitude:</span>
                    <span class="tree-value number">${singleDevice.longitude}</span>
                </div>
                <div class="tree-item">
                    <i class="fas fa-chevron-right tree-toggle"></i>
                    <span class="tree-key">satellites:</span>
                    <span class="tree-value number">${singleDevice.satellites}</span>
                </div>
            `;
        } else {
            devices.forEach((deviceId, index) => {
                const device = data[deviceId];
                const delay = index * 0.1;
                
                tableHTML += `
                    <tr style="animation-delay: ${delay}s">
                        <td>
                            <div class="device-id">
                                <div class="device-icon">
                                    <i class="fas fa-microchip"></i>
                                </div>
                                <div>
                                    <div class="device-name">${deviceId}</div>
                                    <div class="device-id-text">gps_tracking/${deviceId}</div>
                                </div>
                            </div>
                        </td>
                        <td>
                            <div class="coordinate">
                                <span class="coord-value">${device.latitude || '-'}</span>
                                <span class="coord-label">Latitude</span>
                            </div>
                        </td>
                        <td>
                            <div class="coordinate">
                                <span class="coord-value">${device.longitude || '-'}</span>
                                <span class="coord-label">Longitude</span>
                            </div>
                        </td>
                        <td>
                            <span class="stat-badge altitude">
                                <i class="fas fa-mountain"></i>
                                ${device.altitude || '0'} m
                            </span>
                        </td>
                        <td>
                            <span class="stat-badge satellites">
                                <i class="fas fa-satellite"></i>
                                ${device.satellites || '0'}
                            </span>
                        </td>
                    </tr>
                `;
                
                treeHTML += `
                    <div class="tree-item">
                        <i class="fas fa-chevron-right tree-toggle" onclick="this.classList.toggle('expanded'); this.parentElement.querySelector('.tree-children').classList.toggle('collapsed');"></i>
                        <span class="tree-key">${deviceId}</span>
                        <div class="tree-children collapsed">
                            <div class="tree-item">
                                <span class="tree-key">altitude:</span>
                                <span class="tree-value number">${device.altitude || 'null'}</span>
                            </div>
                            <div class="tree-item">
                                <span class="tree-key">latitude:</span>
                                <span class="tree-value number">${device.latitude || 'null'}</span>
                            </div>
                            <div class="tree-item">
                                <span class="tree-key">longitude:</span>
                                <span class="tree-value number">${device.longitude || 'null'}</span>
                            </div>
                            <div class="tree-item">
                                <span class="tree-key">satellites:</span>
                                <span class="tree-value number">${device.satellites || 'null'}</span>
                            </div>
                        </div>
                    </div>
                `;
            });
            
            tableBody.innerHTML = tableHTML;
        }
        
        if (!tableHTML) {
            tableBody.innerHTML = `
                <tr>
                    <td colspan="5">
                        <div class="empty-state">
                            <i class="fas fa-map-marker-alt"></i>
                            <h3>No Devices Found</h3>
                            <p>Add a test device using the Dev Tools panel below</p>
                        </div>
                    </td>
                </tr>
            `;
        }
        
        treeContent.innerHTML = treeHTML || '<div class="tree-item"><span class="tree-key">No data available</span></div>';
    } else {
        tableBody.innerHTML = `
            <tr>
                <td colspan="5">
                    <div class="empty-state">
                        <i class="fas fa-database"></i>
                        <h3>No Data</h3>
                        <p>Waiting for GPS data from connected devices...</p>
                    </div>
                </td>
            </tr>
        `;
        treeContent.innerHTML = '<div class="tree-item"><span class="tree-key">No data available</span></div>';
        deviceCount.textContent = '0 Devices';
    }
}, (error) => {
    isConnected = false;
    connectionStatus.classList.add('disconnected');
    statusText.textContent = 'Disconnected';
    
    console.error('[ERROR] Firebase connection failed!');
    console.error('[ERROR] Error code:', error.code);
    console.error('[ERROR] Error message:', error.message);
    console.error('[ERROR] Full error object:', error);
    
    let errorMessage = error.message;
    if (error.code === 'PERMISSION_DENIED') {
        errorMessage = 'Permission denied. Check Firebase rules.';
        console.error('[ERROR] Hint: Firebase Realtime Database rules need to allow read/write');
    } else if (error.code === 'NETWORK_ERROR') {
        errorMessage = 'Network error. Check internet connection.';
        console.error('[ERROR] Hint: Check firewall or network settings');
    } else if (error.code === 'DISCONNECTED') {
        errorMessage = 'Disconnected from Firebase.';
        console.error('[ERROR] Hint: Server may be down or URL is incorrect');
    }
    
    tableBody.innerHTML = `<tr><td colspan="5" style="color: var(--danger); text-align: center; padding: 40px;"><i class="fas fa-exclamation-triangle"></i> Error: ${errorMessage}</td></tr>`;
});

window.firebaseSetData = async (path, value) => {
    try {
        let targetRef;
        if (typeof path === 'object') {
            targetRef = ref(db, 'gps_tracking');
            await set(targetRef, path);
            console.log('[SUCCESS] Set data at gps_tracking:', path);
        } else {
            const cleanPath = path.startsWith('gps_tracking/') ? path : `gps_tracking/${path}`;
            targetRef = ref(db, cleanPath);
            await set(targetRef, value);
            console.log('[SUCCESS] Set data at:', cleanPath, value);
        }
    } catch (error) {
        console.error('[ERROR] firebaseSetData failed:', error);
        throw error;
    }
};

window.firebaseUpdateData = async (path, value) => {
    try {
        const cleanPath = path.startsWith('/') ? path.slice(1) : path;
        const targetRef = ref(db, cleanPath);
        await update(targetRef, value);
        console.log('[SUCCESS] Updated data at:', cleanPath, value);
    } catch (error) {
        console.error('[ERROR] firebaseUpdateData failed:', error);
        throw error;
    }
};

window.firebaseDeleteData = async (path) => {
    try {
        const cleanPath = path.startsWith('/') ? path.slice(1) : path;
        const targetRef = ref(db, cleanPath);
        await remove(targetRef);
        console.log('[SUCCESS] Deleted data at:', cleanPath);
    } catch (error) {
        console.error('[ERROR] firebaseDeleteData failed:', error);
        throw error;
    }
};

window.firebaseClearAll = async () => {
    try {
        const targetRef = ref(db, 'gps_tracking');
        await remove(targetRef);
        console.log('[SUCCESS] Cleared all data at gps_tracking');
    } catch (error) {
        console.error('[ERROR] firebaseClearAll failed:', error);
        throw error;
    }
};

window.firebaseGetData = () => {
    console.log('[INFO] Retrieved current data from memory');
    return currentData;
};

window.firebaseImportData = async (data) => {
    try {
        const targetRef = ref(db, 'gps_tracking');
        await set(targetRef, data);
        console.log('[SUCCESS] Imported JSON data to gps_tracking:', data);
    } catch (error) {
        console.error('[ERROR] firebaseImportData failed:', error);
        throw error;
    }
};
