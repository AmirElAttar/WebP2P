// ============================================= 
// P2P File Sharing Client - Complete Implementation
// =============================================

// Configuration
const CONFIG = {
  MOCK_MODE: true, // Set to false for real backend
  MOCK_SUCCESS_RATE: 0.9, // 90% success rate
  NETWORK_DELAY: { MIN: 10, MAX: 60 },
  CURRENT_BACKEND: null // Stores connected backend URL
};

// State Management
let localConnected = false;
let backendConnected = false;
let requestCount = 0;
let errorCount = 0;

// Mock Data Stores
let mockSharedFiles = [
  { name: "project_document.pdf", size: 2543216, path: "/shared/docs/project.pdf" },
  { name: "important_notes.txt", size: 2048, path: "/shared/text/notes.txt" }
];

let mockAvailableFiles = [
  { filename: "presentation.pptx", size: 5242880, peers: ["192.168.1.101"], hash: "mock-sha-12345" },
  { filename: "vacation_photos.zip", size: 156743289, peers: ["192.168.1.102"], hash: "mock-sha-67890" }
];

// =============================================
// Core Functions
// =============================================

function log(message, type = 'info') {
  const output = document.getElementById('output');
  const timestamp = new Date().toLocaleTimeString();
  
  const logEntry = document.createElement('div');
  logEntry.className = 'log-entry';
  logEntry.innerHTML = `
    <span class="log-timestamp">[${timestamp}]</span>
    <span class="log-${type}">${message}</span>
  `;
  
  output.appendChild(logEntry);
  output.scrollTop = output.scrollHeight;
  document.getElementById('lastUpdate').textContent = timestamp;
  
  // Update stats
  if (type === 'error') errorCount++;
  requestCount++;
  document.getElementById('statRequests').textContent = requestCount;
  document.getElementById('statErrors').textContent = errorCount;
}

function updateConnectionStatus() {
  // Local service status
  const localIndicator = document.getElementById('localStatusIndicator');
  const localStatus = document.getElementById('localConnectionStatus');
  localIndicator.className = localConnected ? 'status-indicator online' : 'status-indicator offline';
  localStatus.textContent = localConnected ? 'Local Service: Connected' : 'Local Service: Disconnected';
  
  // Backend status
  const backendIndicator = document.getElementById('backendStatusIndicator');
  const backendStatus = document.getElementById('backendConnectionStatus');
  backendIndicator.className = backendConnected ? 'status-indicator online' : 'status-indicator offline';
  backendStatus.textContent = backendConnected ? 'Backend: Connected' : 'Backend: Disconnected';
}

function formatFileSize(bytes) {
  if (bytes === 0) return '0 Bytes';
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2) + ' ' + sizes[i]);
}

// =============================================
// Connection Management
// =============================================

async function testLocalConnection() {
  const serviceUrl = document.getElementById('localServiceUrl').value.trim();
  if (!serviceUrl) {
    log("⚠️ Please enter local service URL", 'warning');
    return false;
  }

  log(`🔌 Testing connection to: ${serviceUrl}`, 'info');
  
  if (CONFIG.MOCK_MODE) {
    try {
      // Simulate connection test
      await new Promise(resolve => setTimeout(resolve, 500));
      localConnected = true;
      updateConnectionStatus();
      log("✅ Local service connected (MOCK)", 'success');
      return true;
    } catch (error) {
      localConnected = false;
      updateConnectionStatus();
      log("❌ Local connection failed (MOCK)", 'error');
      return false;
    }
  } else {
    try {
      // Real implementation
      const response = await fetch(`${serviceUrl}/api/status`);
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      
      const data = await response.json();
      localConnected = true;
      updateConnectionStatus();
      
      // Update stats
      document.getElementById('statPort').textContent = data.port || '8847';
      document.getElementById('statUptime').textContent = data.uptime || '0';
      
      log(`✅ Local service connected! Uptime: ${data.uptime}s`, 'success');
      return true;
    } catch (error) {
      localConnected = false;
      updateConnectionStatus();
      log(`❌ Local connection failed: ${error.message}`, 'error');
      return false;
    }
  }
}

async function connectBackend() {
  const backendUrl = document.getElementById('backendUrl').value.trim();
  if (!backendUrl) {
    log("⚠️ Please enter backend URL", 'warning');
    return false;
  }

  log(`🌍 Connecting to backend: ${backendUrl}`, 'info');
  
  if (CONFIG.MOCK_MODE) {
    try {
      // Simulate connection with success rate
      await new Promise((resolve, reject) => {
        setTimeout(() => {
          if (Math.random() < CONFIG.MOCK_SUCCESS_RATE) {
            CONFIG.CURRENT_BACKEND = backendUrl;
            backendConnected = true;
            updateConnectionStatus();
            log(`✅ Backend connected (MOCK)`, 'success');
            resolve();
          } else {
            reject(new Error("Connection timed out"));
          }
        }, 800);
      });
      
      // Load files after successful connection
      await refreshAvailableFiles();
      return true;
    } catch (error) {
      backendConnected = false;
      updateConnectionStatus();
      log(`❌ Backend connection failed: ${error.message}`, 'error');
      return false;
    }
  } else {
    try {
      // Real implementation
      const response = await fetch(`${backendUrl}/api/status`);
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      
      const data = await response.json();
      CONFIG.CURRENT_BACKEND = backendUrl;
      backendConnected = true;
      updateConnectionStatus();
      
      log(`✅ Backend connected! Files available: ${data.files || 0}`, 'success');
      await refreshAvailableFiles();
      return true;
    } catch (error) {
      backendConnected = false;
      updateConnectionStatus();
      log(`❌ Backend connection failed: ${error.message}`, 'error');
      return false;
    }
  }
}

function disconnectBackend() {
  backendConnected = false;
  CONFIG.CURRENT_BACKEND = null;
  updateConnectionStatus();
  document.getElementById('availableFilesList').innerHTML = '';
  log("🌍 Disconnected from backend", 'info');
}

// =============================================
// File Operations
// =============================================

async function registerSharedFiles() {
  if (!backendConnected) {
    log("⚠️ Please connect to backend first", 'warning');
    return false;
  }

  try {
    // Get shared files from local service
    const serviceUrl = document.getElementById('localServiceUrl').value.trim();
    let localFiles;
    
    if (CONFIG.MOCK_MODE) {
      localFiles = {
        files: [...mockSharedFiles]
      };
    } else {
      const response = await fetch(`${serviceUrl}/api/files`);
      localFiles = await response.json();
    }
    
    if (!localFiles.files || localFiles.files.length === 0) {
      log("ℹ️ No files to register", 'info');
      return false;
    }

    // Register each file
    for (const file of localFiles.files) {
      try {
        let fileHash;
        if (CONFIG.MOCK_MODE) {
          fileHash = `mock-sha-${file.name.replace(/\.[^/.]+$/, "")}-${file.size}`;
        } else {
          // Real SHA-256 calculation would go here
          fileHash = "real-sha-to-be-implemented";
        }
        
        const registerData = {
          filename: file.name,
          size: file.size,
          hash: fileHash
        };
        
        if (CONFIG.MOCK_MODE) {
          // Add to mock available files
          mockAvailableFiles.push({
            filename: file.name,
            size: file.size,
            peers: [getMockLocalIP()],
            hash: fileHash
          });
        } else {
          // Real registration
          await fetch(`${CONFIG.CURRENT_BACKEND}/api/register`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(registerData)
          });
        }
        
        log(`📝 Registered: ${file.name} (${formatFileSize(file.size)})`, 'success');
      } catch (error) {
        log(`⚠️ Failed to register ${file.name}: ${error.message}`, 'warning');
      }
    }
    
    // Refresh available files list
    await refreshAvailableFiles();
    return true;
  } catch (error) {
    log(`❌ File registration failed: ${error.message}`, 'error');
    return false;
  }
}

async function refreshAvailableFiles() {
  if (!backendConnected) {
    log("⚠️ Please connect to backend first", 'warning');
    return false;
  }

  try {
    let availableFiles;
    
    if (CONFIG.MOCK_MODE) {
      availableFiles = {
        files: [...mockAvailableFiles]
      };
    } else {
      const response = await fetch(`${CONFIG.CURRENT_BACKEND}/api/files`);
      availableFiles = await response.json();
    }
    
    const filesList = document.getElementById('availableFilesList');
    filesList.innerHTML = '';
    
    if (!availableFiles.files || availableFiles.files.length === 0) {
      filesList.innerHTML = '<li class="no-files">No files available</li>';
      log("ℹ️ No files available on backend", 'info');
      return true;
    }
    
    log(`📁 Found ${availableFiles.files.length} available files`, 'success');
    
    availableFiles.files.forEach(file => {
      const li = document.createElement('li');
      li.className = 'file-item';
      li.innerHTML = `
        <div class="file-info">
          <div class="file-name">${file.filename}</div>
          <div class="file-size">${formatFileSize(file.size)}</div>
          <div class="file-peers">${file.peers?.length || 1} peer(s)</div>
        </div>
        <button class="btn download-btn" 
                onclick="downloadFile('${file.filename}', '${file.peers?.[0] || getMockLocalIP()}')">
          Download
        </button>
      `;
      filesList.appendChild(li);
    });
    
    return true;
  } catch (error) {
    log(`❌ Failed to refresh files: ${error.message}`, 'error');
    return false;
  }
}

async function downloadFile(filename, peerUrl) {
  log(`📥 Starting download: ${filename} from ${peerUrl}`, 'info');
  
  try {
    // Create progress element
    const progressId = `progress-${filename.replace(/\W/g, '_')}`;
    log(`⏳ Download progress: <div class="progress"><div id="${progressId}" class="progress-bar" style="width:0%"></div></div>`, 'info');
    
    // Simulate or perform download
    if (CONFIG.MOCK_MODE) {
      for (let progress = 0; progress <= 100; progress += 10) {
        await new Promise(resolve => setTimeout(resolve, 200));
        document.getElementById(progressId).style.width = `${progress}%`;
      }
      
      // Simulate file content
      const fileData = {
        filename: filename,
        size: Math.floor(Math.random() * 10000000),
        data: `Content of ${filename}`
      };
      
      log(`✅ Download complete: ${filename} (${formatFileSize(fileData.size)})`, 'success');
    } else {
      // Real download implementation
      // This would be replaced with actual P2P transfer code
      const response = await fetch(`${peerUrl}/download/${filename}`);
      const blob = await response.blob();
      
      // Save file
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      
      log(`✅ Download complete: ${filename}`, 'success');
    }
    
    return true;
  } catch (error) {
    log(`❌ Download failed: ${error.message}`, 'error');
    return false;
  }
}

// =============================================
// Utility Functions
// =============================================

function getMockLocalIP() {
  return `192.168.1.${Math.floor(100 + Math.random() * 155)}`;
}

function clearLogs() {
  document.getElementById('output').innerHTML = '';
  requestCount = 0;
  errorCount = 0;
  document.getElementById('statRequests').textContent = '0';
  document.getElementById('statErrors').textContent = '0';
  log("Logs cleared", 'info');
}

// =============================================
// Initialization
// =============================================

async function initializeApp() {
  // Set default URLs
  document.getElementById('localServiceUrl').value = "http://localhost:8847";
  document.getElementById('backendUrl').value = "http://123.45.67.89:8000";
  
  // Display mode indicator
  const modeIndicator = document.createElement('div');
  modeIndicator.id = 'mode-indicator';
  modeIndicator.textContent = CONFIG.MOCK_MODE ? "MOCK MODE" : "LIVE MODE";
  modeIndicator.className = CONFIG.MOCK_MODE ? 'mock-mode' : 'live-mode';
  document.querySelector('.container').prepend(modeIndicator);
  
  // Initialize UI with shared files
  const sharedList = document.getElementById('sharedFilesList');
  mockSharedFiles.forEach(file => {
    const li = document.createElement('li');
    li.className = 'file-item';
    li.innerHTML = `
      <div class="file-info">
        <div class="file-name">${file.name}</div>
        <div class="file-size">${formatFileSize(file.size)}</div>
      </div>
    `;
    sharedList.appendChild(li);
  });

  // Test local connection on startup
  await testLocalConnection();
  log("🚀 P2P client initialized", 'success');
}

// Initialize when page loads
window.onload = initializeApp;

// =============================================
// CSS for Progress Bars (Add to your styles)
// =============================================
const progressStyle = document.createElement('style');
progressStyle.textContent = `
  .progress {
    width: 100%;
    height: 6px;
    background: #e2e8f0;
    border-radius: 3px;
    margin-top: 5px;
    overflow: hidden;
  }
  .progress-bar {
    height: 100%;
    background: #48bb78;
    transition: width 0.3s ease;
  }
  #mode-indicator {
    position: absolute;
    top: 15px;
    right: 15px;
    padding: 5px 10px;
    border-radius: 4px;
    font-weight: bold;
    font-size: 12px;
  }
  .mock-mode {
    background: #e53e3e;
    color: white;
  }
  .live-mode {
    background: #48bb78;
    color: white;
  }
`;
document.head.appendChild(progressStyle);
