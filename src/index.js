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
        const keys = Object.keys(data);
        console.log('[INFO] Data keys:', keys);
        
        const flatFields = ['latitude', 'longitude', 'altitude', 'satellites'];
        const isFlatFormat = flatFields.every(field => keys.includes(field)) &&
                             !keys.some(k => typeof data[k] === 'object');
        
        if (isFlatFormat) {
            console.log('[INFO] Rendering FLAT format (1 ESP32 Device)');
            deviceCount.textContent = 'ESP32 Device';
            
            tableBody.innerHTML = `
                <tr>
                    <td>
                        <div class="device-id">
                            <div class="device-icon"><i class="fas fa-microchip"></i></div>
                            <div>
                                <div class="device-name">ESP32 Device</div>
                                <div class="device-id-text">gps_tracking</div>
                            </div>
                        </div>
                    </td>
                    <td><div class="coordinate"><span class="coord-value">${data.latitude || '-'}</span></div></td>
                    <td><div class="coordinate"><span class="coord-value">${data.longitude || '-'}</span></div></td>
                    <td><span class="stat-badge altitude"><i class="fas fa-mountain"></i> ${data.altitude || '0'} m</span></td>
                    <td><span class="stat-badge satellites"><i class="fas fa-satellite"></i> ${data.satellites || '0'}</span></td>
                </tr>
            `;
            
            treeContent.innerHTML = `
                <div class="tree-item"><span class="key">latitude:</span> <span class="value">${data.latitude}</span></div>
                <div class="tree-item"><span class="key">longitude:</span> <span class="value">${data.longitude}</span></div>
                <div class="tree-item"><span class="key">altitude:</span> <span class="value">${data.altitude}</span></div>
                <div class="tree-item"><span class="key">satellites:</span> <span class="value">${data.satellites}</span></div>
            `;
        } else {
            console.log('[INFO] Rendering as multiple entries');
            deviceCount.textContent = `${keys.length} Entry`;
            
            let tableHTML = '';
            let treeHTML = '';
            
            keys.forEach((key, index) => {
                const value = data[key];
                const delay = index * 0.1;
                const isObject = typeof value === 'object' && value !== null;
                
                const displayValue = isObject ? JSON.stringify(value) : value;
                
                tableHTML += `
                    <tr style="animation-delay: ${delay}s">
                        <td>
                            <div class="device-id">
                                <div class="device-icon"><i class="fas fa-database"></i></div>
                                <div>
                                    <div class="device-name">${key}</div>
                                    <div class="device-id-text">gps_tracking/${key}</div>
                                </div>
                            </div>
                        </td>
                        <td colspan="4"><code>${displayValue}</code></td>
                    </tr>
                `;
            });
            
            tableBody.innerHTML = tableHTML;
            
            treeHTML = keys.map(key => {
                const value = data[key];
                const isObject = typeof value === 'object' && value !== null;
                if (isObject) {
                    const children = Object.entries(value).map(([k, v]) => 
                        `<div class="tree-item"><span class="key">${k}:</span> <span class="value">${v}</span></div>`
                    ).join('');
                    return `<div class="tree-item"><span class="key">${key}:</span>${children}</div>`;
                }
                return `<div class="tree-item"><span class="key">${key}:</span> <span class="value">${value}</span></div>`;
            }).join('');
            
            treeContent.innerHTML = treeHTML;
        }
    } else {
        console.log('[INFO] No data available');
        deviceCount.textContent = 'No Data';
        tableBody.innerHTML = `
            <tr>
                <td colspan="5">
                    <div class="empty-state">
                        <i class="fas fa-database"></i>
                        <h3>No Data</h3>
                        <p>Waiting for GPS data from ESP32...</p>
                    </div>
                </td>
            </tr>
        `;
        treeContent.innerHTML = '<div class="tree-item"><span class="tree-key">No data available</span></div>';
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

window.DevTools = {
    panelCollapsed: true,
    
    togglePanel() {
        const panel = document.getElementById('devtoolsPanel');
        if (!panel) {
            console.error('[ERROR] devtoolsPanel element not found');
            return;
        }
        panel.classList.toggle('collapsed');
        this.panelCollapsed = panel.classList.contains('collapsed');
        console.log('[INFO] DevTools panel', this.panelCollapsed ? 'collapsed' : 'expanded');
    },

    showToast(message, type = 'info') {
        console.log(`[${type.toUpperCase()}] ${message}`);
        const container = document.getElementById('toastContainer');
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        
        const icons = {
            success: 'fas fa-check-circle',
            error: 'fas fa-exclamation-circle',
            warning: 'fas fa-exclamation-triangle',
            info: 'fas fa-info-circle'
        };
        
        toast.innerHTML = `
            <i class="${icons[type]}"></i>
            <span class="toast-message">${message}</span>
            <span class="toast-time">${new Date().toLocaleTimeString()}</span>
        `;
        
        container.appendChild(toast);
        setTimeout(() => toast.remove(), 3000);
    },

    async setRandomCoords() {
        console.log('[INFO] DevTools: Setting random coordinates...');
        try {
            const lat = (Math.random() * 180 - 90).toFixed(6);
            const lng = (Math.random() * 360 - 180).toFixed(6);
            const alt = Math.floor(Math.random() * 500);
            const sat = Math.floor(Math.random() * 20) + 1;
            
            console.log(`[INFO] Generated random coords: lat=${lat}, lng=${lng}, alt=${alt}, sat=${sat}`);
            
            await window.firebaseSetData({
                latitude: parseFloat(lat),
                longitude: parseFloat(lng),
                altitude: alt,
                satellites: sat
            });
            
            this.showToast('Random coordinates set successfully!', 'success');
            console.log('[SUCCESS] Random coordinates set');
        } catch (error) {
            console.error('[ERROR] setRandomCoords failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async simulateGPSUpdate() {
        console.log('[INFO] DevTools: Simulating GPS update...');
        try {
            const testData = {
                latitude: -6.2088 + (Math.random() * 0.01 - 0.005),
                longitude: 106.8456 + (Math.random() * 0.01 - 0.005),
                altitude: Math.floor(Math.random() * 200),
                satellites: Math.floor(Math.random() * 15) + 5
            };
            
            console.log('[INFO] Simulating GPS data:', testData);
            await window.firebaseSetData(testData);
            
            document.getElementById('editValue').value = JSON.stringify(testData, null, 2);
            this.showToast('GPS data simulated!', 'success');
            console.log('[SUCCESS] GPS simulation complete');
        } catch (error) {
            console.error('[ERROR] simulateGPSUpdate failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async clearAllData() {
        console.log('[INFO] DevTools: Clearing all data...');
        if (!confirm('Are you sure you want to clear all data? This cannot be undone!')) {
            console.log('[INFO] Clear all cancelled by user');
            return;
        }
        
        try {
            await window.firebaseClearAll();
            this.showToast('All data cleared successfully!', 'success');
            console.log('[SUCCESS] All data cleared');
        } catch (error) {
            console.error('[ERROR] clearAllData failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async setData() {
        console.log('[INFO] DevTools: Setting GPS data...');
        const value = document.getElementById('editValue').value;
        
        if (!value) {
            console.warn('[WARNING] No data specified');
            this.showToast('Please enter GPS JSON data!', 'warning');
            return;
        }
        
        try {
            const parsedValue = JSON.parse(value);
            console.log('[INFO] Setting flat GPS data:', parsedValue);
            await window.firebaseSetData(parsedValue);
            this.showToast('GPS data set!', 'success');
            console.log('[SUCCESS] GPS data set');
        } catch (error) {
            console.error('[ERROR] Invalid JSON:', error);
            this.showToast('Invalid JSON format!', 'error');
        }
    },

    async updateData() {
        console.log('[INFO] DevTools: Updating data...');
        const path = document.getElementById('updatePath').value.trim();
        const value = document.getElementById('updateValue').value;
        
        if (!path) {
            console.warn('[WARNING] No path specified');
            this.showToast('Please enter a path!', 'warning');
            return;
        }
        
        try {
            const parsedValue = parseFloat(value) || value;
            console.log(`[INFO] Updating data: path="${path}", value=${parsedValue}`);
            
            await window.firebaseUpdateData(path, parsedValue);
            this.showToast(`Updated "${path}"!`, 'success');
            console.log('[SUCCESS] Data updated:', path);
        } catch (error) {
            console.error('[ERROR] updateData failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async deleteData() {
        console.log('[INFO] DevTools: Deleting data...');
        const path = document.getElementById('updatePath').value.trim();
        
        if (!path) {
            console.warn('[WARNING] No path specified');
            this.showToast('Please enter a path to delete!', 'warning');
            return;
        }
        
        if (!confirm(`Delete "${path}"?`)) {
            console.log('[INFO] Delete cancelled by user');
            return;
        }
        
        try {
            await window.firebaseDeleteData(path);
            this.showToast(`Deleted "${path}"!`, 'success');
            console.log('[SUCCESS] Data deleted:', path);
        } catch (error) {
            console.error('[ERROR] deleteData failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async exportJSON() {
        console.log('[INFO] DevTools: Exporting JSON...');
        try {
            const data = window.firebaseGetData();
            if (data) {
                const json = JSON.stringify(data, null, 2);
                console.log('[INFO] Exported data size:', json.length, 'characters');
                
                const blob = new Blob([json], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `gps_data_${Date.now()}.json`;
                a.click();
                URL.revokeObjectURL(url);
                this.showToast('Data exported successfully!', 'success');
                console.log('[SUCCESS] JSON exported');
            } else {
                console.warn('[WARNING] No data to export');
                this.showToast('No data to export!', 'warning');
            }
        } catch (error) {
            console.error('[ERROR] exportJSON failed:', error);
            this.showToast('Error: ' + error.message, 'error');
        }
    },

    async importJSON() {
        console.log('[INFO] DevTools: Importing JSON...');
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        
        input.onchange = async (e) => {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = async (event) => {
                    try {
                        const data = JSON.parse(event.target.result);
                        console.log('[INFO] Parsed JSON data:', data);
                        await window.firebaseImportData(data);
                        this.showToast('Data imported successfully!', 'success');
                        console.log('[SUCCESS] JSON imported');
                    } catch (error) {
                        console.error('[ERROR] Invalid JSON format:', error);
                        this.showToast('Invalid JSON format!', 'error');
                    }
                };
                reader.readAsText(file);
            }
        };
        
        input.click();
    },

    updateJsonPreview(data) {
        const preview = document.getElementById('jsonPreview');
        if (data) {
            const json = JSON.stringify(data, null, 2);
            preview.innerHTML = this.syntaxHighlight(json);
        } else {
            preview.innerHTML = '<span style="color: var(--text-secondary);">No data loaded...</span>';
        }
    },

    syntaxHighlight(json) {
        json = json.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
        return json.replace(/(".*?"):\s*(-?\d+\.?\d*)/g, '<span class="json-key">$1</span>: <span class="json-number">$2</span>')
                   .replace(/(".*?"):\s*"(.*?)"/g, '<span class="json-key">$1</span>: <span class="json-string">"$2"</span>')
                   .replace(/(".*?"):\s*(true|false)/g, '<span class="json-key">$1</span>: <span class="json-boolean">$2</span>')
                   .replace(/(".*?"):\s*(null)/g, '<span class="json-key">$1</span>: <span class="json-boolean">$2</span>');
    }
};

document.addEventListener('DOMContentLoaded', () => {
    const toggleBtn = document.getElementById('devtoolsToggle');
    if (toggleBtn) {
        toggleBtn.addEventListener('click', () => DevTools.togglePanel());
        console.log('[INFO] DevTools toggle listener attached successfully');
    } else {
        console.error('[ERROR] devtoolsToggle element not found');
    }
});

window.addEventListener('error', (event) => {
    console.error('[UNHANDLED ERROR]', event.error);
});

window.addEventListener('unhandledrejection', (event) => {
    console.error('[UNHANDLED PROMISE REJECTION]', event.reason);
});

console.log('[INFO] DevTools initialized successfully');
