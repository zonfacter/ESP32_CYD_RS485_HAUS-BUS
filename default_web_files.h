/**
 * default_web_files.h - Version 2.0
 * 
 * Standard Web-Interface Dateien für ESP32 Touch-Panel
 * Diese Dateien werden automatisch erstellt, falls sie nicht im LittleFS vorhanden sind
 */

#ifndef DEFAULT_WEB_FILES_H
#define DEFAULT_WEB_FILES_H

// HTML-Dateien
const char* DEFAULT_INDEX_HTML = R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Touch Panel - Dashboard</title>
    <link rel="stylesheet" href="/web/style.css">
    <link rel="icon" href="data:image/x-icon;base64,">
</head>
<body>
    <div class="container">
        <header>
            <h1>🎛️ ESP32 Touch Panel</h1>
            <div class="status-indicator" id="connectionStatus">
                <span class="status-dot offline"></span>
                <span>Verbinde...</span>
            </div>
        </header>

        <nav class="nav-tabs">
            <button class="tab-button active" data-tab="dashboard">Dashboard</button>
            <button class="tab-button" data-tab="buttons">Buttons</button>
            <button class="tab-button" data-tab="system">System</button>
            <button class="tab-button" data-tab="logs">Logs</button>
            <button class="tab-button" data-tab="files">Dateien</button>
        </nav>

        <!-- Dashboard Tab -->
        <div class="tab-content active" id="dashboard">
            <div class="grid">
                <div class="card">
                    <h3>📊 System-Status</h3>
                    <div class="status-grid">
                        <div class="status-item">
                            <label>Device ID:</label>
                            <span id="deviceId">-</span>
                        </div>
                        <div class="status-item">
                            <label>Uptime:</label>
                            <span id="uptime">-</span>
                        </div>
                        <div class="status-item">
                            <label>Freier Speicher:</label>
                            <span id="freeHeap">-</span>
                        </div>
                        <div class="status-item">
                            <label>WiFi:</label>
                            <span id="wifiStatus">-</span>
                        </div>
                    </div>
                </div>

                <div class="card">
                    <h3>🔘 Button-Status</h3>
                    <div class="button-grid" id="buttonStatus">
                        <!-- Buttons werden dynamisch geladen -->
                    </div>
                </div>

                <div class="card">
                    <h3>📡 CSMA/CD Statistiken</h3>
                    <div class="stats-grid">
                        <div class="stat-item">
                            <div class="stat-value" id="totalSent">0</div>
                            <div class="stat-label">Gesendet</div>
                        </div>
                        <div class="stat-item">
                            <div class="stat-value" id="totalCollisions">0</div>
                            <div class="stat-label">Kollisionen</div>
                        </div>
                        <div class="stat-item">
                            <div class="stat-value" id="totalRetries">0</div>
                            <div class="stat-label">Wiederholungen</div>
                        </div>
                        <div class="stat-item">
                            <div class="stat-value" id="queueCount">0</div>
                            <div class="stat-label">Warteschlange</div>
                        </div>
                    </div>
                </div>

                <div class="card">
                    <h3>⚡ Schnellaktionen</h3>
                    <div class="action-buttons">
                        <button class="btn btn-success" onclick="sendTelegram()">📤 Test-Telegramm</button>
                        <button class="btn btn-warning" onclick="restartDevice()">🔄 Neustart</button>
                        <button class="btn btn-info" onclick="createBackup()">💾 Backup</button>
                        <button class="btn btn-danger" onclick="factoryReset()">🏭 Factory Reset</button>
                    </div>
                </div>
            </div>
        </div>

        <!-- Buttons Tab -->
        <div class="tab-content" id="buttons">
            <div class="card">
                <h3>🎛️ Button-Konfiguration</h3>
                <div class="form-grid">
                    <div class="form-group">
                        <label>Button auswählen:</label>
                        <select id="buttonSelect" onchange="loadButtonConfig()">
                            <option value="0">Button 1 (ID: 17)</option>
                            <option value="1">Button 2 (ID: 18)</option>
                            <option value="2">Button 3 (ID: 19)</option>
                            <option value="3">Button 4 (ID: 20)</option>
                            <option value="4">Button 5 (ID: 21)</option>
                            <option value="5">Button 6 (ID: 22)</option>
                        </select>
                    </div>
                    
                    <div class="form-group">
                        <label>Label:</label>
                        <input type="text" id="buttonLabel" placeholder="Button-Beschriftung">
                    </div>
                    
                    <div class="form-group">
                        <label>Farbe:</label>
                        <input type="color" id="buttonColor" value="#808080">
                    </div>
                    
                    <div class="form-group">
                        <label>Aktiviert:</label>
                        <input type="checkbox" id="buttonEnabled" checked>
                    </div>
                    
                    <div class="form-group">
                        <label>Priorität:</label>
                        <input type="number" id="buttonPriority" min="0" max="9" value="5">
                    </div>
                </div>
                
                <div class="action-buttons">
                    <button class="btn btn-success" onclick="saveButtonConfig()">💾 Speichern</button>
                    <button class="btn btn-secondary" onclick="testButton()">🧪 Test</button>
                </div>
            </div>
            
            <div class="card">
                <h3>🎮 Button-Steuerung</h3>
                <div class="button-control-grid" id="buttonControls">
                    <!-- Button-Controls werden dynamisch erstellt -->
                </div>
            </div>
        </div>

        <!-- System Tab -->
        <div class="tab-content" id="system">
            <div class="grid">
                <div class="card">
                    <h3>⚙️ Gerätekonfiguration</h3>
                    <div class="form-grid">
                        <div class="form-group">
                            <label>Device ID:</label>
                            <input type="text" id="configDeviceId" maxlength="4" pattern="[0-9]{4}">
                        </div>
                        
                        <div class="form-group">
                            <label>Orientierung:</label>
                            <select id="configOrientation">
                                <option value="0">Portrait</option>
                                <option value="1">Landscape</option>
                            </select>
                        </div>
                        
                        <div class="form-group">
                            <label>Helligkeit (%):</label>
                            <input type="range" id="configBrightness" min="0" max="100" value="100">
                            <span id="brightnessValue">100%</span>
                        </div>
                        
                        <div class="form-group">
                            <label>Debug-Modus:</label>
                            <input type="checkbox" id="configDebugMode">
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>🌐 Netzwerk-Konfiguration</h3>
                    <div class="form-grid">
                        <div class="form-group">
                            <label>AP SSID:</label>
                            <input type="text" id="configApSsid">
                        </div>
                        
                        <div class="form-group">
                            <label>AP Passwort:</label>
                            <input type="password" id="configApPassword">
                        </div>
                        
                        <div class="form-group">
                            <label>Hostname:</label>
                            <input type="text" id="configHostname">
                        </div>
                        
                        <div class="form-group">
                            <label>Web-Server Port:</label>
                            <input type="number" id="configWebPort" min="80" max="65535" value="80">
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>📡 CSMA/CD Einstellungen</h3>
                    <div class="form-grid">
                        <div class="form-group">
                            <label>Bus Idle Time (ms):</label>
                            <input type="number" id="configBusIdleTime" min="1" max="100" value="10">
                        </div>
                        
                        <div class="form-group">
                            <label>Max. Wiederholungen:</label>
                            <input type="number" id="configMaxRetries" min="1" max="10" value="5">
                        </div>
                        
                        <div class="form-group">
                            <label>Sendepuffer-Größe:</label>
                            <input type="number" id="configQueueSize" min="5" max="50" value="10">
                        </div>
                    </div>
                </div>
            </div>
            
            <div class="action-buttons">
                <button class="btn btn-success" onclick="saveSystemConfig()">💾 Konfiguration speichern</button>
                <button class="btn btn-secondary" onclick="loadSystemConfig()">🔄 Neu laden</button>
            </div>
        </div>

        <!-- Logs Tab -->
        <div class="tab-content" id="logs">
            <div class="card">
                <h3>📋 System-Logs</h3>
                <div class="log-controls">
                    <select id="logLevel">
                        <option value="all">Alle Levels</option>
                        <option value="info">Info</option>
                        <option value="warning">Warning</option>
                        <option value="error">Error</option>
                        <option value="debug">Debug</option>
                    </select>
                    <button class="btn btn-info" onclick="refreshLogs()">🔄 Aktualisieren</button>
                    <button class="btn btn-warning" onclick="clearLogs()">🗑️ Löschen</button>
                    <button class="btn btn-success" onclick="downloadLogs()">💾 Download</button>
                </div>
                <pre class="log-viewer" id="logContent">Logs werden geladen...</pre>
            </div>
        </div>

        <!-- Files Tab -->
        <div class="tab-content" id="files">
            <div class="card">
                <h3>📁 Datei-Manager</h3>
                <div class="file-controls">
                    <input type="file" id="fileUpload" multiple style="display: none;">
                    <button class="btn btn-success" onclick="uploadFiles()">📤 Upload</button>
                    <button class="btn btn-info" onclick="refreshFiles()">🔄 Aktualisieren</button>
                    <button class="btn btn-warning" onclick="createBackup()">💾 Backup erstellen</button>
                </div>
                
                <div class="file-stats">
                    <span>Speicher: <span id="fileSystemUsage">-</span></span>
                    <span>Dateien: <span id="fileCount">-</span></span>
                </div>
                
                <div class="file-list" id="fileList">
                    <!-- Dateiliste wird dynamisch geladen -->
                </div>
            </div>
        </div>
    </div>

    <footer>
        <p>ESP32 Touch Panel v2.0 | LittleFS Web-Interface | 
           Letztes Update: <span id="lastUpdate">-</span>
        </p>
    </footer>

    <script src="/web/script.js"></script>
</body>
</html>
)HTML";

const char* DEFAULT_STYLE_CSS = R"CSS(
/* ESP32 Touch Panel Web-Interface - Modern Dark Theme */
:root {
    --primary-color: #007bff;
    --secondary-color: #6c757d;
    --success-color: #28a745;
    --warning-color: #ffc107;
    --danger-color: #dc3545;
    --info-color: #17a2b8;
    --light-color: #f8f9fa;
    --dark-color: #343a40;
    --bg-color: #1a1a1a;
    --card-bg: #2d2d2d;
    --text-color: #ffffff;
    --text-muted: #b0b0b0;
    --border-color: #404040;
    --shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background-color: var(--bg-color);
    color: var(--text-color);
    line-height: 1.6;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 20px;
}

/* Header */
header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 2rem;
    padding-bottom: 1rem;
    border-bottom: 2px solid var(--border-color);
}

header h1 {
    font-size: 2rem;
    color: var(--primary-color);
}

.status-indicator {
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.status-dot {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    animation: pulse 2s infinite;
}

.status-dot.online {
    background-color: var(--success-color);
}

.status-dot.offline {
    background-color: var(--danger-color);
}

@keyframes pulse {
    0% { opacity: 1; }
    50% { opacity: 0.5; }
    100% { opacity: 1; }
}

/* Navigation Tabs */
.nav-tabs {
    display: flex;
    gap: 0.5rem;
    margin-bottom: 2rem;
    border-bottom: 1px solid var(--border-color);
}

.tab-button {
    background: none;
    border: none;
    color: var(--text-muted);
    padding: 1rem 1.5rem;
    cursor: pointer;
    transition: all 0.3s ease;
    border-bottom: 3px solid transparent;
}

.tab-button:hover {
    color: var(--text-color);
    background-color: rgba(255, 255, 255, 0.05);
}

.tab-button.active {
    color: var(--primary-color);
    border-bottom-color: var(--primary-color);
}

/* Tab Content */
.tab-content {
    display: none;
}

.tab-content.active {
    display: block;
}

/* Grid Layouts */
.grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
    margin-bottom: 2rem;
}

.form-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 1rem;
}

.status-grid, .stats-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 1rem;
}

/* Cards */
.card {
    background: var(--card-bg);
    border-radius: 8px;
    padding: 1.5rem;
    box-shadow: var(--shadow);
    border: 1px solid var(--border-color);
}

.card h3 {
    margin-bottom: 1rem;
    color: var(--primary-color);
    font-size: 1.2rem;
}

/* Form Elements */
.form-group {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}

.form-group label {
    font-weight: 600;
    color: var(--text-color);
}

input, select, textarea {
    background: var(--bg-color);
    border: 1px solid var(--border-color);
    border-radius: 4px;
    padding: 0.75rem;
    color: var(--text-color);
    font-size: 0.9rem;
}

input:focus, select:focus, textarea:focus {
    outline: none;
    border-color: var(--primary-color);
    box-shadow: 0 0 0 2px rgba(0, 123, 255, 0.25);
}

input[type="range"] {
    padding: 0;
}

input[type="checkbox"] {
    width: auto;
    margin-right: 0.5rem;
}

/* Buttons */
.btn {
    background: var(--primary-color);
    color: white;
    border: none;
    padding: 0.75rem 1.5rem;
    border-radius: 4px;
    cursor: pointer;
    font-size: 0.9rem;
    transition: all 0.3s ease;
    display: inline-flex;
    align-items: center;
    gap: 0.5rem;
}

.btn:hover {
    transform: translateY(-1px);
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

.btn-success { background: var(--success-color); }
.btn-warning { background: var(--warning-color); color: var(--dark-color); }
.btn-danger { background: var(--danger-color); }
.btn-info { background: var(--info-color); }
.btn-secondary { background: var(--secondary-color); }

.action-buttons {
    display: flex;
    gap: 1rem;
    flex-wrap: wrap;
    margin-top: 1rem;
}

/* Status Items */
.status-item {
    display: flex;
    justify-content: space-between;
    padding: 0.5rem 0;
    border-bottom: 1px solid var(--border-color);
}

.status-item:last-child {
    border-bottom: none;
}

.status-item label {
    font-weight: 600;
    color: var(--text-muted);
}

/* Statistics */
.stat-item {
    text-align: center;
    padding: 1rem;
    background: rgba(255, 255, 255, 0.05);
    border-radius: 4px;
}

.stat-value {
    font-size: 2rem;
    font-weight: bold;
    color: var(--primary-color);
}

.stat-label {
    font-size: 0.8rem;
    color: var(--text-muted);
    margin-top: 0.5rem;
}

/* Button Grid */
.button-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
    gap: 1rem;
}

.button-status {
    padding: 1rem;
    text-align: center;
    border-radius: 4px;
    border: 2px solid var(--border-color);
    transition: all 0.3s ease;
}

.button-status.active {
    border-color: var(--success-color);
    background: rgba(40, 167, 69, 0.1);
}

.button-status.inactive {
    border-color: var(--secondary-color);
    background: rgba(108, 117, 125, 0.1);
}

/* Button Controls */
.button-control-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 1rem;
}

.button-control {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 1rem;
    background: rgba(255, 255, 255, 0.05);
    border-radius: 4px;
    border: 1px solid var(--border-color);
}

/* Log Viewer */
.log-controls {
    display: flex;
    gap: 1rem;
    margin-bottom: 1rem;
    align-items: center;
}

.log-viewer {
    background: var(--bg-color);
    border: 1px solid var(--border-color);
    border-radius: 4px;
    padding: 1rem;
    max-height: 400px;
    overflow-y: auto;
    font-family: 'Courier New', monospace;
    font-size: 0.8rem;
    white-space: pre-wrap;
    color: var(--text-color);
}

/* File Manager */
.file-controls {
    display: flex;
    gap: 1rem;
    margin-bottom: 1rem;
    align-items: center;
}

.file-stats {
    display: flex;
    gap: 2rem;
    margin-bottom: 1rem;
    font-size: 0.9rem;
    color: var(--text-muted);
}

.file-list {
    border: 1px solid var(--border-color);
    border-radius: 4px;
    overflow: hidden;
}

.file-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1rem;
    border-bottom: 1px solid var(--border-color);
    transition: background-color 0.3s ease;
}

.file-item:hover {
    background: rgba(255, 255, 255, 0.05);
}

.file-item:last-child {
    border-bottom: none;
}

.file-info {
    display: flex;
    flex-direction: column;
    gap: 0.25rem;
}

.file-name {
    font-weight: 600;
}

.file-size {
    font-size: 0.8rem;
    color: var(--text-muted);
}

.file-actions {
    display: flex;
    gap: 0.5rem;
}

.file-actions .btn {
    padding: 0.25rem 0.5rem;
    font-size: 0.8rem;
}

/* Progress Bar */
.progress {
    width: 100%;
    height: 8px;
    background: var(--border-color);
    border-radius: 4px;
    overflow: hidden;
    margin: 1rem 0;
}

.progress-bar {
    height: 100%;
    background: var(--primary-color);
    transition: width 0.3s ease;
}

/* Footer */
footer {
    margin-top: 3rem;
    padding: 2rem 0;
    border-top: 1px solid var(--border-color);
    text-align: center;
    color: var(--text-muted);
    font-size: 0.9rem;
}

/* Responsive Design */
@media (max-width: 768px) {
    .container {
        padding: 1rem;
    }
    
    header {
        flex-direction: column;
        gap: 1rem;
        text-align: center;
    }
    
    .nav-tabs {
        flex-wrap: wrap;
    }
    
    .tab-button {
        padding: 0.75rem 1rem;
        font-size: 0.9rem;
    }
    
    .grid {
        grid-template-columns: 1fr;
    }
    
    .form-grid {
        grid-template-columns: 1fr;
    }
    
    .action-buttons {
        justify-content: center;
    }
    
    .file-item {
        flex-direction: column;
        align-items: flex-start;
        gap: 1rem;
    }
    
    .file-actions {
        width: 100%;
        justify-content: center;
    }
}

/* Animations */
@keyframes fadeIn {
    from { opacity: 0; transform: translateY(20px); }
    to { opacity: 1; transform: translateY(0); }
}

.card, .tab-content {
    animation: fadeIn 0.5s ease-out;
}

/* Scrollbar Styling */
::-webkit-scrollbar {
    width: 8px;
}

::-webkit-scrollbar-track {
    background: var(--bg-color);
}

::-webkit-scrollbar-thumb {
    background: var(--border-color);
    border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
    background: var(--text-muted);
}

/* Loading Animation */
.loading {
    display: inline-block;
    width: 20px;
    height: 20px;
    border: 3px solid var(--border-color);
    border-radius: 50%;
    border-top-color: var(--primary-color);
    animation: spin 1s ease-in-out infinite;
}

@keyframes spin {
    to { transform: rotate(360deg); }
}

/* Notifications */
.notification {
    position: fixed;
    top: 20px;
    right: 20px;
    padding: 1rem 1.5rem;
    border-radius: 4px;
    color: white;
    font-weight: 600;
    z-index: 1000;
    animation: slideIn 0.3s ease-out;
}

.notification.success { background: var(--success-color); }
.notification.error { background: var(--danger-color); }
.notification.warning { background: var(--warning-color); color: var(--dark-color); }
.notification.info { background: var(--info-color); }

@keyframes slideIn {
    from { transform: translateX(100%); }
    to { transform: translateX(0); }
}
)CSS";

const char* DEFAULT_SCRIPT_JS = R"JS(
/**
 * ESP32 Touch Panel Web-Interface JavaScript
 * Version 2.0 - LittleFS Edition
 */

class TouchPanelAPI {
    constructor() {
        this.baseURL = window.location.origin;
        this.updateInterval = 5000; // 5 Sekunden
        this.lastUpdate = null;
        this.connected = false;
        
        this.init();
    }
    
    init() {
        this.setupEventListeners();
        this.startAutoUpdate();
        this.loadInitialData();
    }
    
    setupEventListeners() {
        // Tab-Navigation
        document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                this.switchTab(e.target.dataset.tab);
            });
        });
        
        // Form-Events
        document.getElementById('brightnessValue')?.addEventListener('input', (e) => {
            document.getElementById('brightnessValue').textContent = e.target.value + '%';
        });
        
        // File Upload
        document.getElementById('fileUpload')?.addEventListener('change', (e) => {
            this.handleFileUpload(e.target.files);
        });
        
        // Live-Updates für Slider
        document.getElementById('configBrightness')?.addEventListener('input', (e) => {
            document.getElementById('brightnessValue').textContent = e.target.value + '%';
            this.updateBrightness(e.target.value);
        });
    }
    
    switchTab(tabName) {
        // Alle Tabs deaktivieren
        document.querySelectorAll('.tab-button').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
        
        // Aktiven Tab aktivieren
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
        document.getElementById(tabName).classList.add('active');
        
        // Spezielle Tab-Initialisierung
        switch(tabName) {
            case 'buttons':
                this.loadButtonConfig();
                this.loadButtonControls();
                break;
            case 'system':
                this.loadSystemConfig();
                break;
            case 'logs':
                this.refreshLogs();
                break;
            case 'files':
                this.refreshFiles();
                break;
        }
    }
    
    async apiCall(endpoint, method = 'GET', data = null) {
        try {
            const options = {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                }
            };
            
            if (data) {
                options.body = JSON.stringify(data);
            }
            
            const response = await fetch(`${this.baseURL}/api${endpoint}`, options);
            const result = await response.json();
            
            if (!response.ok) {
                throw new Error(result.message || 'API-Fehler');
            }
            
            this.updateConnectionStatus(true);
            return result;
        } catch (error) {
            console.error('API-Fehler:', error);
            this.updateConnectionStatus(false);
            this.showNotification('Verbindungsfehler: ' + error.message, 'error');
            throw error;
        }
    }
    
    updateConnectionStatus(connected) {
        this.connected = connected;
        const statusElement = document.getElementById('connectionStatus');
        const dot = statusElement.querySelector('.status-dot');
        const text = statusElement.querySelector('span:last-child');
        
        if (connected) {
            dot.className = 'status-dot online';
            text.textContent = 'Verbunden';
        } else {
            dot.className = 'status-dot offline';
            text.textContent = 'Getrennt';
        }
    }
    
    async loadInitialData() {
        try {
            await Promise.all([
                this.updateDashboard(),
                this.loadSystemConfig(),
                this.loadButtonControls()
            ]);
        } catch (error) {
            console.error('Fehler beim Laden der Initialdaten:', error);
        }
    }
    
    async updateDashboard() {
        try {
            const status = await this.apiCall('/status');
            
            // System-Status aktualisieren
            document.getElementById('deviceId').textContent = status.data.device.deviceID;
            document.getElementById('uptime').textContent = this.formatUptime(status.data.system.uptime);
            document.getElementById('freeHeap').textContent = this.formatBytes(status.data.system.freeHeap);
            document.getElementById('wifiStatus').textContent = status.data.network.connected ? 'Verbunden' : 'Getrennt';
            
            // CSMA/CD Statistiken
            if (status.data.csma) {
                document.getElementById('totalSent').textContent = status.data.csma.totalSent;
                document.getElementById('totalCollisions').textContent = status.data.csma.totalCollisions;
                document.getElementById('totalRetries').textContent = status.data.csma.totalRetries;
                document.getElementById('queueCount').textContent = status.data.csma.queueCount;
            }
            
            // Button-Status
            this.updateButtonStatus(status.data.buttons);
            
            this.lastUpdate = new Date();
            document.getElementById('lastUpdate').textContent = this.lastUpdate.toLocaleTimeString();
            
        } catch (error) {
            console.error('Dashboard-Update fehlgeschlagen:', error);
        }
    }
    
    updateButtonStatus(buttons) {
        const container = document.getElementById('buttonStatus');
        if (!container) return;
        
        container.innerHTML = '';
        
        buttons.forEach((button, index) => {
            const buttonDiv = document.createElement('div');
            buttonDiv.className = `button-status ${button.active ? 'active' : 'inactive'}`;
            buttonDiv.innerHTML = `
                <div class="button-name">${button.label}</div>
                <div class="button-id">ID: ${button.instanceID}</div>
                <div class="button-state">${button.active ? 'Aktiv' : 'Inaktiv'}</div>
            `;
            container.appendChild(buttonDiv);
        });
    }
    
    async loadSystemConfig() {
        try {
            const config = await this.apiCall('/config');
            
            // Device Config
            document.getElementById('configDeviceId').value = config.data.device.deviceID;
            document.getElementById('configOrientation').value = config.data.device.orientation;
            document.getElementById('configBrightness').value = config.data.display.brightness;
            document.getElementById('brightnessValue').textContent = config.data.display.brightness + '%';
            document.getElementById('configDebugMode').checked = config.data.device.debugMode;
            
            // Network Config
            document.getElementById('configApSsid').value = config.data.network.apSSID;
            document.getElementById('configApPassword').value = config.data.network.apPassword;
            document.getElementById('configHostname').value = config.data.network.hostname;
            document.getElementById('configWebPort').value = config.data.network.webServerPort;
            
            // CSMA Config
            document.getElementById('configBusIdleTime').value = config.data.csma.busIdleTime;
            document.getElementById('configMaxRetries').value = config.data.csma.maxRetriesPerTelegram;
            document.getElementById('configQueueSize').value = config.data.csma.sendQueueSize;
            
        } catch (error) {
            console.error('Konfiguration laden fehlgeschlagen:', error);
        }
    }
    
    async saveSystemConfig() {
        try {
            const config = {
                device: {
                    deviceID: document.getElementById('configDeviceId').value,
                    orientation: parseInt(document.getElementById('configOrientation').value),
                    debugMode: document.getElementById('configDebugMode').checked
                },
                network: {
                    apSSID: document.getElementById('configApSsid').value,
                    apPassword: document.getElementById('configApPassword').value,
                    hostname: document.getElementById('configHostname').value,
                    webServerPort: parseInt(document.getElementById('configWebPort').value)
                },
                display: {
                    brightness: parseInt(document.getElementById('configBrightness').value)
                },
                csma: {
                    busIdleTime: parseInt(document.getElementById('configBusIdleTime').value),
                    maxRetriesPerTelegram: parseInt(document.getElementById('configMaxRetries').value),
                    sendQueueSize: parseInt(document.getElementById('configQueueSize').value)
                }
            };
            
            await this.apiCall('/config', 'POST', config);
            this.showNotification('Konfiguration gespeichert', 'success');
            
        } catch (error) {
            this.showNotification('Fehler beim Speichern: ' + error.message, 'error');
        }
    }
    
    async loadButtonConfig() {
        const buttonIndex = document.getElementById('buttonSelect').value;
        try {
            const config = await this.apiCall(`/buttons/${buttonIndex}`);
            
            document.getElementById('buttonLabel').value = config.data.label;
            document.getElementById('buttonColor').value = config.data.color;
            document.getElementById('buttonEnabled').checked = config.data.enabled;
            document.getElementById('buttonPriority').value = config.data.priority;
            
        } catch (error) {
            console.error('Button-Konfiguration laden fehlgeschlagen:', error);
        }
    }
    
    async saveButtonConfig() {
        const buttonIndex = document.getElementById('buttonSelect').value;
        try {
            const config = {
                label: document.getElementById('buttonLabel').value,
                color: document.getElementById('buttonColor').value,
                enabled: document.getElementById('buttonEnabled').checked,
                priority: parseInt(document.getElementById('buttonPriority').value)
            };
            
            await this.apiCall(`/buttons/${buttonIndex}`, 'POST', config);
            this.showNotification('Button-Konfiguration gespeichert', 'success');
            this.loadButtonControls(); // Controls aktualisieren
            
        } catch (error) {
            this.showNotification('Fehler beim Speichern: ' + error.message, 'error');
        }
    }
    
    async loadButtonControls() {
        try {
            const config = await this.apiCall('/config');
            const container = document.getElementById('buttonControls');
            if (!container) return;
            
            container.innerHTML = '';
            
            config.data.buttons.buttons.forEach((button, index) => {
                const controlDiv = document.createElement('div');
                controlDiv.className = 'button-control';
                controlDiv.innerHTML = `
                    <div>
                        <div class="button-name">${button.label}</div>
                        <div class="button-id">ID: ${button.instanceID}</div>
                    </div>
                    <div class="button-actions">
                        <button class="btn btn-success btn-sm" onclick="api.activateButton(${index})">Ein</button>
                        <button class="btn btn-secondary btn-sm" onclick="api.deactivateButton(${index})">Aus</button>
                        <button class="btn btn-info btn-sm" onclick="api.testButton(${index})">Test</button>
                    </div>
                `;
                container.appendChild(controlDiv);
            });
            
        } catch (error) {
            console.error('Button-Controls laden fehlgeschlagen:', error);
        }
    }
    
    async activateButton(buttonIndex) {
        try {
            await this.apiCall(`/buttons/${buttonIndex}/activate`, 'POST');
            this.showNotification(`Button ${buttonIndex + 1} aktiviert`, 'success');
            this.updateDashboard();
        } catch (error) {
            this.showNotification('Fehler beim Aktivieren: ' + error.message, 'error');
        }
    }
    
    async deactivateButton(buttonIndex) {
        try {
            await this.apiCall(`/buttons/${buttonIndex}/deactivate`, 'POST');
            this.showNotification(`Button ${buttonIndex + 1} deaktiviert`, 'success');
            this.updateDashboard();
        } catch (error) {
            this.showNotification('Fehler beim Deaktivieren: ' + error.message, 'error');
        }
    }
    
    async testButton(buttonIndex) {
        try {
            await this.apiCall(`/buttons/${buttonIndex}/test`, 'POST');
            this.showNotification(`Button ${buttonIndex + 1} getestet`, 'info');
        } catch (error) {
            this.showNotification('Test fehlgeschlagen: ' + error.message, 'error');
        }
    }
    
    async updateBrightness(value) {
        try {
            await this.apiCall('/display/brightness', 'POST', { brightness: parseInt(value) });
        } catch (error) {
            console.error('Helligkeit-Update fehlgeschlagen:', error);
        }
    }
    
    async refreshLogs() {
        try {
            const level = document.getElementById('logLevel').value;
            const logs = await this.apiCall(`/logs?level=${level}`);
            document.getElementById('logContent').textContent = logs.data.content;
        } catch (error) {
            document.getElementById('logContent').textContent = 'Fehler beim Laden der Logs: ' + error.message;
        }
    }
    
    async clearLogs() {
        if (confirm('Alle Logs löschen?')) {
            try {
                await this.apiCall('/logs', 'DELETE');
                this.showNotification('Logs gelöscht', 'success');
                this.refreshLogs();
            } catch (error) {
                this.showNotification('Fehler beim Löschen: ' + error.message, 'error');
            }
        }
    }
    
    async downloadLogs() {
        try {
            const response = await fetch(`${this.baseURL}/api/logs/download`);
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `esp32-logs-${new Date().toISOString().split('T')[0]}.txt`;
            a.click();
            window.URL.revokeObjectURL(url);
        } catch (error) {
            this.showNotification('Download fehlgeschlagen: ' + error.message, 'error');
        }
    }
    
    async refreshFiles() {
        try {
            const files = await this.apiCall('/files');
            const container = document.getElementById('fileList');
            if (!container) return;
            
            container.innerHTML = '';
            
            // Filesystem-Stats aktualisieren
            document.getElementById('fileSystemUsage').textContent = 
                `${this.formatBytes(files.data.stats.usedBytes)} / ${this.formatBytes(files.data.stats.totalBytes)} (${files.data.stats.usagePercent.toFixed(1)}%)`;
            document.getElementById('fileCount').textContent = files.data.files.length;
            
            files.data.files.forEach(file => {
                const fileDiv = document.createElement('div');
                fileDiv.className = 'file-item';
                fileDiv.innerHTML = `
                    <div class="file-info">
                        <div class="file-name">${file.name}</div>
                        <div class="file-size">${this.formatBytes(file.size)}</div>
                    </div>
                    <div class="file-actions">
                        <button class="btn btn-info" onclick="api.downloadFile('${file.path}')">Download</button>
                        <button class="btn btn-danger" onclick="api.deleteFile('${file.path}')">Löschen</button>
                    </div>
                `;
                container.appendChild(fileDiv);
            });
            
        } catch (error) {
            console.error('Dateiliste laden fehlgeschlagen:', error);
        }
    }
    
    async uploadFiles() {
        document.getElementById('fileUpload').click();
    }
    
    async handleFileUpload(files) {
        for (const file of files) {
            try {
                const formData = new FormData();
                formData.append('file', file);
                
                const response = await fetch(`${this.baseURL}/api/files/upload`, {
                    method: 'POST',
                    body: formData
                });
                
                if (response.ok) {
                    this.showNotification(`${file.name} hochgeladen`, 'success');
                } else {
                    throw new Error('Upload fehlgeschlagen');
                }
            } catch (error) {
                this.showNotification(`Upload von ${file.name} fehlgeschlagen: ${error.message}`, 'error');
            }
        }
        
        this.refreshFiles();
    }
    
    async downloadFile(filepath) {
        try {
            const response = await fetch(`${this.baseURL}/api/files/download?path=${encodeURIComponent(filepath)}`);
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filepath.split('/').pop();
            a.click();
            window.URL.revokeObjectURL(url);
        } catch (error) {
            this.showNotification('Download fehlgeschlagen: ' + error.message, 'error');
        }
    }
    
    async deleteFile(filepath) {
        if (confirm(`Datei ${filepath} löschen?`)) {
            try {
                await this.apiCall(`/files/delete?path=${encodeURIComponent(filepath)}`, 'DELETE');
                this.showNotification('Datei gelöscht', 'success');
                this.refreshFiles();
            } catch (error) {
                this.showNotification('Löschen fehlgeschlagen: ' + error.message, 'error');
            }
        }
    }
    
    async sendTelegram() {
        const telegram = prompt('Telegramm eingeben (Format: FUNCTION.INSTANCE.ACTION.PARAMS):');
        if (telegram) {
            try {
                await this.apiCall('/telegram', 'POST', { telegram: telegram });
                this.showNotification('Telegramm gesendet', 'success');
            } catch (error) {
                this.showNotification('Senden fehlgeschlagen: ' + error.message, 'error');
            }
        }
    }
    
    async restartDevice() {
        if (confirm('Gerät wirklich neu starten?')) {
            try {
                await this.apiCall('/restart', 'POST');
                this.showNotification('Neustart wird eingeleitet...', 'warning');
                
                // Reconnect nach 10 Sekunden versuchen
                setTimeout(() => {
                    window.location.reload();
                }, 10000);
            } catch (error) {
                this.showNotification('Neustart fehlgeschlagen: ' + error.message, 'error');
            }
        }
    }
    
    async createBackup() {
        try {
            const backupName = prompt('Backup-Name (optional):') || 
                              `backup-${new Date().toISOString().split('T')[0]}`;
            
            await this.apiCall('/backup', 'POST', { name: backupName });
            this.showNotification('Backup erstellt', 'success');
            this.refreshFiles();
        } catch (error) {
            this.showNotification('Backup fehlgeschlagen: ' + error.message, 'error');
        }
    }
    
    async factoryReset() {
        const confirmation = prompt('ACHTUNG: Factory Reset löscht alle Daten!\nGeben Sie "RESET" ein um fortzufahren:');
        if (confirmation === 'RESET') {
            try {
                await this.apiCall('/factory-reset', 'POST');
                this.showNotification('Factory Reset durchgeführt', 'warning');
                
                // Seite nach 5 Sekunden neu laden
                setTimeout(() => {
                    window.location.reload();
                }, 5000);
            } catch (error) {
                this.showNotification('Factory Reset fehlgeschlagen: ' + error.message, 'error');
            }
        }
    }
    
    startAutoUpdate() {
        setInterval(() => {
            if (this.connected && document.querySelector('.tab-content.active').id === 'dashboard') {
                this.updateDashboard();
            }
        }, this.updateInterval);
    }
    
    showNotification(message, type = 'info') {
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;
        
        document.body.appendChild(notification);
        
        setTimeout(() => {
            notification.remove();
        }, 5000);
    }
    
    formatBytes(bytes) {
        if (bytes === 0) return '0 Bytes';
        
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
    
    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        
        if (days > 0) {
            return `${days}d ${hours}h ${minutes}m`;
        } else if (hours > 0) {
            return `${hours}h ${minutes}m`;
        } else {
            return `${minutes}m`;
        }
    }
}

// Globale API-Instanz
const api = new TouchPanelAPI();

// Globale Funktionen für Event-Handler
function sendTelegram() { api.sendTelegram(); }
function restartDevice() { api.restartDevice(); }
function createBackup() { api.createBackup(); }
function factoryReset() { api.factoryReset(); }
function loadButtonConfig() { api.loadButtonConfig(); }
function saveButtonConfig() { api.saveButtonConfig(); }
function testButton() { 
    const buttonIndex = document.getElementById('buttonSelect').value;
    api.testButton(parseInt(buttonIndex)); 
}
function saveSystemConfig() { api.saveSystemConfig(); }
function loadSystemConfig() { api.loadSystemConfig(); }
function refreshLogs() { api.refreshLogs(); }
function clearLogs() { api.clearLogs(); }
function downloadLogs() { api.downloadLogs(); }
function uploadFiles() { api.uploadFiles(); }
function refreshFiles() { api.refreshFiles(); }

// Service Worker für Offline-Funktionalität (optional)
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('/sw.js')
            .then(registration => {
                console.log('SW registered: ', registration);
            })
            .catch(registrationError => {
                console.log('SW registration failed: ', registrationError);
            });
    });
}

// Keyboard-Shortcuts
document.addEventListener('keydown', (e) => {
    if (e.ctrlKey || e.metaKey) {
        switch(e.key) {
            case '1': api.switchTab('dashboard'); e.preventDefault(); break;
            case '2': api.switchTab('buttons'); e.preventDefault(); break;
            case '3': api.switchTab('system'); e.preventDefault(); break;
            case '4': api.switchTab('logs'); e.preventDefault(); break;
            case '5': api.switchTab('files'); e.preventDefault(); break;
            case 'r': api.refreshLogs(); e.preventDefault(); break;
            case 's': api.saveSystemConfig(); e.preventDefault(); break;
        }
    }
});

// Visibility API für Auto-Update Pausierung
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        console.log('Tab hidden - pausing auto-updates');
    } else {
        console.log('Tab visible - resuming auto-updates');
        api.updateDashboard();
    }
});

console.log('ESP32 Touch Panel Web-Interface v2.0 geladen');
)JS";

#endif // DEFAULT_WEB_FILES_H