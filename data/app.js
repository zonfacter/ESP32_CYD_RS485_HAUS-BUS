/**
 * ESP32 Touch Panel - Haupt-JavaScript-Datei
 * 
 * Gemeinsame Funktionen für alle Seiten:
 * - Nachrichten-System
 * - Formatierungs-Hilfsfunktionen
 * - API-Kommunikation
 * - Utility-Funktionen
 */

// === GLOBALE VARIABLEN ===
let messageTimeout = null;

// === NACHRICHTEN-SYSTEM ===
function showMessage(message, type = 'info', duration = 5000) {
    const container = document.getElementById('messageContainer');
    if (!container) {
        console.warn('Message container nicht gefunden');
        return;
    }

    // Vorherige Nachrichten entfernen
    clearMessages();

    // Neue Nachricht erstellen
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${type}`;
    messageDiv.innerHTML = `
        <span>${message}</span>
        <span onclick="this.parentElement.remove()" style="float: right; cursor: pointer; font-weight: bold;">&times;</span>
    `;

    // Nachricht hinzufügen
    container.appendChild(messageDiv);

    // Auto-Remove nach Timeout
    if (duration > 0) {
        messageTimeout = setTimeout(() => {
            messageDiv.classList.add('fade-out');
            setTimeout(() => {
                if (messageDiv.parentNode) {
                    messageDiv.parentNode.removeChild(messageDiv);
                }
            }, 300);
        }, duration);
    }

    // Click-to-remove
    messageDiv.addEventListener('click', () => {
        clearTimeout(messageTimeout);
        messageDiv.remove();
    });
}

function clearMessages() {
    const container = document.getElementById('messageContainer');
    if (container) {
        container.innerHTML = '';
    }
    clearTimeout(messageTimeout);
}

// === API-KOMMUNIKATION ===
function apiCall(endpoint, method = 'GET', data = null) {
    const options = {
        method: method,
        headers: {}
    };

    if (data) {
        if (method === 'POST') {
            options.headers['Content-Type'] = 'application/x-www-form-urlencoded';
            options.body = new URLSearchParams(data).toString();
        }
    }

    return fetch(endpoint, options)
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            return response.json();
        })
        .catch(error => {
            console.error('API-Fehler:', error);
            showMessage('API-Fehler: ' + error.message, 'error');
            throw error;
        });
}

function getStatus() {
    return apiCall('/api/status');
}

function getConfig() {
    return apiCall('/api/config');
}

function saveConfig(config) {
    return apiCall('/api/config', 'POST', config);
}

// === FORMATIERUNGS-HILFSFUNKTIONEN ===
function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${secs}s`;
    } else {
        return `${secs}s`;
    }
}

function formatTimestamp(timestamp) {
    const date = new Date(timestamp);
    return date.toLocaleString('de-DE', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
}

// === UTILITY-FUNKTIONEN ===
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

function throttle(func, limit) {
    let inThrottle;
    return function() {
        const args = arguments;
        const context = this;
        if (!inThrottle) {
            func.apply(context, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}

// === VALIDIERUNGS-FUNKTIONEN ===
function validateDeviceID(id) {
    return /^\d{4}$/.test(id);
}

function validateBrightness(value) {
    const num = parseInt(value);
    return !isNaN(num) && num >= 0 && num <= 100;
}

function validateOrientation(value) {
    return value === '0' || value === '1' || value === 0 || value === 1;
}

// === LOKALER SPEICHER ===
function saveToLocalStorage(key, value) {
    try {
        localStorage.setItem('esp32_' + key, JSON.stringify(value));
        return true;
    } catch (error) {
        console.warn('LocalStorage nicht verfügbar:', error);
        return false;
    }
}

function loadFromLocalStorage(key, defaultValue = null) {
    try {
        const item = localStorage.getItem('esp32_' + key);
        return item ? JSON.parse(item) : defaultValue;
    } catch (error) {
        console.warn('LocalStorage nicht verfügbar:', error);
        return defaultValue;
    }
}

// === KONFIGURATIONS-MANAGEMENT ===
function loadUserPreferences() {
    const preferences = loadFromLocalStorage('preferences', {
        autoRefresh: true,
        refreshInterval: 5000,
        showNotifications: true,
        theme: 'auto'
    });
    
    return preferences;
}

function saveUserPreferences(preferences) {
    return saveToLocalStorage('preferences', preferences);
}

// === NETZWERK-STATUS ===
function checkConnectionStatus() {
    return fetch('/api/status', { 
        method: 'HEAD',
        cache: 'no-cache'
    })
    .then(response => response.ok)
    .catch(() => false);
}

let connectionCheckInterval = null;

function startConnectionMonitoring() {
    if (connectionCheckInterval) return;
    
    connectionCheckInterval = setInterval(async () => {
        const isOnline = await checkConnectionStatus();
        updateConnectionStatus(isOnline);
    }, 10000); // Alle 10 Sekunden prüfen
}

function stopConnectionMonitoring() {
    if (connectionCheckInterval) {
        clearInterval(connectionCheckInterval);
        connectionCheckInterval = null;
    }
}

function updateConnectionStatus(isOnline) {
    const statusElements = document.querySelectorAll('.connection-status');
    statusElements.forEach(element => {
        if (isOnline) {
            element.textContent = 'Online';
            element.className = 'connection-status status-ok';
        } else {
            element.textContent = 'Offline';
            element.className = 'connection-status status-error';
        }
    });
    
    if (!isOnline) {
        showMessage('Verbindung zum ESP32 verloren', 'warning');
    }
}

// === KEYBOARD-SHORTCUTS ===
function setupKeyboardShortcuts() {
    document.addEventListener('keydown', (event) => {
        // Strg+R: Manuelle Aktualisierung
        if (event.ctrlKey && event.key === 'r') {
            event.preventDefault();
            if (typeof updateDashboard === 'function') {
                updateDashboard();
                showMessage('Dashboard aktualisiert', 'info');
            }
        }
        
        // Strg+S: Konfiguration speichern (falls auf Config-Seite)
        if (event.ctrlKey && event.key === 's') {
            event.preventDefault();
            if (typeof saveConfiguration === 'function') {
                saveConfiguration();
            }
        }
        
        // ESC: Nachrichten schließen
        if (event.key === 'Escape') {
            clearMessages();
        }
    });
}

// === INITIALISIERUNG ===
document.addEventListener('DOMContentLoaded', function() {
    console.log('ESP32 Touch Panel Web-Interface geladen');
    
    // Message Container erstellen falls nicht vorhanden
    if (!document.getElementById('messageContainer')) {
        const container = document.createElement('div');
        container.id = 'messageContainer';
        document.body.appendChild(container);
    }
    
    // Benutzereinstellungen laden
    const preferences = loadUserPreferences();
    
    // Verbindungsüberwachung starten
    startConnectionMonitoring();
    
    // Keyboard-Shortcuts aktivieren
    setupKeyboardShortcuts();
    
    // Willkommensnachricht
    setTimeout(() => {
        showMessage('Web-Interface erfolgreich geladen', 'success', 3000);
    }, 1000);
});

// === CLEANUP BEI SEITENWECHSEL ===
window.addEventListener('beforeunload', function() {
    stopConnectionMonitoring();
    clearMessages();
});

// === EXPORT FÜR MODULE ===
window.ESP32TouchPanel = {
    // API
    apiCall,
    getStatus,
    getConfig,
    saveConfig,
    
    // UI
    showMessage,
    clearMessages,
    
    // Formatierung
    formatBytes,
    formatUptime,
    formatTimestamp,
    
    // Utilities
    debounce,
    throttle,
    validateDeviceID,
    validateBrightness,
    validateOrientation,
    
    // Storage
    saveToLocalStorage,
    loadFromLocalStorage,
    loadUserPreferences,
    saveUserPreferences,
    
    // Monitoring
    startConnectionMonitoring,
    stopConnectionMonitoring,
    checkConnectionStatus
};