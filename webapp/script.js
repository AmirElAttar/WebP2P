
        // =============================================
        // Configuration
        // =============================================
        const CONFIG = {
            MOCK_MODE: true, // Set to false for real backend
            MOCK_SUCCESS_RATE: 0.9, // 90% success rate for mock
            NETWORK_DELAY: { MIN: 10, MAX: 100 } // Simulated network latency
        };
        
        // State Management
        let localConnected = false;
        let backendConnected = false;
        let requestCount = 0;
        let errorCount = 0;
        
        // Mock Data Stores
        const mockSharedFiles = [
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
            requestCount++;
            if (type === 'error') errorCount++;
            document.getElementById('statRequests').textContent = requestCount;
            document.getElementById('statErrors').textContent = errorCount;
        }
        
        function updateConnectionStatus() {
            // Update local service status
            const localIndicator = document.getElementById('localStatusIndicator');
            const localStatus = document.getElementById('localConnectionStatus');
            localIndicator.className = localConnected ? 'status-indicator online' : 'status-indicator offline';
            localStatus.textContent = localConnected ? 'Local Service: Connected' : 'Local Service: Disconnected';
            
            // Update backend status
            const backendIndicator = document.getElementById('backendStatusIndicator');
            const backendStatus = document.getElementById('backendConnectionStatus');
            backendIndicator.className = backendConnected ? 'status-indicator online' : 'status-indicator offline';
            backendStatus.textContent = backendConnected ? 'Backend: Connected' : 'Backend: Disconnected';
            
            // Update mode badge
            const modeBadge = document.getElementById('modeBadge');
            modeBadge.className = CONFIG.MOCK_MODE ? 'mode-badge mock-mode' : 'mode-badge real-mode';
            modeBadge.textContent = CONFIG.MOCK_MODE ? 'MOCK MODE' : 'REAL MODE';
        }
        
        function formatFileSize(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }
        
        function getLocalServiceUrl() {
            return document.getElementById('localServiceUrl').value.trim() || "http://localhost:8847";
        }
        
        function getBackendUrl() {
            return document.getElementById('backendUrl').value.trim() || "http://123.45.67.89:8000";
        }
        
        function getMockLocalIP() {
            return `192.168.1.${100 + Math.floor(Math.random() * 155)}`;
        }
        
        // =============================================
        // Connection Management
        // =============================================
        
        async function testLocalConnection() {
            const serviceUrl = getLocalServiceUrl();
            log(`Testing connection to local service: ${serviceUrl}`);
            
            if (CONFIG.MOCK_MODE) {
                // Mock implementation
                try {
                    log("Simulating local connection...");
                    
                    // Simulate network delay
                    await new Promise(resolve => 
                        setTimeout(resolve, 300 + Math.random() * 500)
                    );
                    
                    // Random success/failure
                    if (Math.random() < CONFIG.MOCK_SUCCESS_RATE) {
                        localConnected = true;
                        updateConnectionStatus();
                        log("✅ Local service connected (MOCK)", 'success');
                    } else {
                        throw new Error("Connection timed out");
                    }
                } catch (error) {
                    localConnected = false;
                    updateConnectionStatus();
                    log(`❌ Local connection failed: ${error.message}`, 'error');
                }
            } else {
                // Real implementation
                try {
                    const response = await fetch(`${serviceUrl}/api/status`);
                    
                    if (response.ok) {
                        const data = await response.json();
                        localConnected = true;
                        updateConnectionStatus();
                        log(`✅ Local service connected! Status: ${data.status}`, 'success');
                    } else {
                        throw new Error(`HTTP ${response.status}`);
                    }
                } catch (error) {
                    localConnected = false;
                    updateConnectionStatus();
                    log(`❌ Local connection failed: ${error.message}`, 'error');
                }
            }
        }
        
        async function connectBackend() {
            const backendUrl = getBackendUrl();
            log(`Connecting to backend: ${backendUrl}`);
            
            if (CONFIG.MOCK_MODE) {
                // Mock implementation
                try {
                    log("Simulating backend connection...");
                    
                    // Simulate network delay
                    await new Promise(resolve => 
                        setTimeout(resolve, 400 + Math.random() * 700)
                    );
                    
                    // Random success/failure
                    if (Math.random() < CONFIG.MOCK_SUCCESS_RATE) {
                        backendConnected = true;
                        updateConnectionStatus();
                        log("✅ Backend connected (MOCK)", 'success');
                        
                        // Load available files after connection
                        refreshAvailableFiles();
                    } else {
                        throw new Error("Connection refused");
                    }
                } catch (error) {
                    backendConnected = false;
                    updateConnectionStatus();
                    log(`❌ Backend connection failed: ${error.message}`, 'error');
                }
            } else {
                // Real implementation
                try {
                    const response = await fetch(`${backendUrl}/api/status`);
                    
                    if (response.ok) {
                        const data = await response.json();
                        backendConnected = true;
                        updateConnectionStatus();
                        log(`✅ Backend connected! Peers: ${data.peers || 0}`, 'success');
                        
                        // Load available files after connection
                        refreshAvailableFiles();
                    } else {
                        throw new Error(`HTTP ${response.status}`);
                    }
                } catch (error) {
                    backendConnected = false;
                    updateConnectionStatus();
                    log(`❌ Backend connection failed: ${error.message}`, 'error');
                }
            }
        }
        
        function disconnectBackend() {
            backendConnected = false;
            updateConnectionStatus();
            document.getElementById('availableFilesList').innerHTML = '';
            log("Disconnected from backend", 'info');
        }
        
        // =============================================
        // File Operations
        // =============================================
        
        async function registerSharedFiles() {
            if (!backendConnected) {
                log("⚠️ Please connect to backend first", 'warning');
                return;
            }
            
            if (CONFIG.MOCK_MODE) {
                // Mock implementation
                try {
                    log("Registering shared files with backend (MOCK)...");
                    
                    // Simulate delay
                    await new Promise(resolve => 
                        setTimeout(resolve, 500 + Math.random() * 500)
                    );
                    
                    // Process each file
                    for (const file of mockSharedFiles) {
                        const fileHash = `mock-sha-${file.name.replace(/\.[^/.]+$/, "")}-${file.size}`;
                        const newFile = {
                            filename: file.name,
                            size: file.size,
                            peers: [getMockLocalIP()],
                            hash: fileHash
                        };
                        
                        mockAvailableFiles.push(newFile);
                        log(`✅ Registered: ${file.name} (${formatFileSize(file.size)})`, 'success');
                    }
                    
                    // Refresh available files
                    refreshAvailableFiles();
                } catch (error) {
                    log(`❌ Registration failed: ${error.message}`, 'error');
                }
            } else {
                // Real implementation
                try {
                    log("Registering shared files with backend...");
                    
                    // Get shared files from local service
                    const serviceUrl = getLocalServiceUrl();
                    const response = await fetch(`${serviceUrl}/api/files`);
                    const data = await response.json();
                    
                    if (!data.files || data.files.length === 0) {
                        log("ℹ️ No files to register", 'info');
                        return;
                    }
                    
                    // Register each file
                    for (const file of data.files) {
                        try {
                            // In a real implementation, we would calculate the SHA hash here
                            const fileHash = "real-sha-to-be-calculated";
                            
                            // Send registration to backend
                            const backendUrl = getBackendUrl();
                            const registerResponse = await fetch(`${backendUrl}/api/register`, {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({
                                    filename: file.name,
                                    size: file.size,
                                    hash: fileHash,
                                    peer_url: serviceUrl
                                })
                            });
                            
                            if (registerResponse.ok) {
                                log(`✅ Registered: ${file.name}`, 'success');
                            } else {
                                throw new Error(`HTTP ${registerResponse.status}`);
                            }
                        } catch (error) {
                            log(`⚠️ Failed to register ${file.name}: ${error.message}`, 'warning');
                        }
                    }
                    
                    // Refresh available files
                    refreshAvailableFiles();
                } catch (error) {
                    log(`❌ Registration failed: ${error.message}`, 'error');
                }
            }
        }
        
        async function refreshAvailableFiles() {
            if (!backendConnected) {
                log("⚠️ Please connect to backend first", 'warning');
                return;
            }
            
            if (CONFIG.MOCK_MODE) {
                // Mock implementation
                try {
                    log("Refreshing available files (MOCK)...");
                    
                    // Simulate delay
                    await new Promise(resolve => 
                        setTimeout(resolve, 400 + Math.random() * 600)
                    );
                    
                    const listElement = document.getElementById('availableFilesList');
                    listElement.innerHTML = '';
                    
                    if (mockAvailableFiles.length === 0) {
                        listElement.innerHTML = '<li class="no-files">No files available</li>';
                        log("ℹ️ No files available on backend", 'info');
                        return;
                    }
                    
                    log(`📁 Found ${mockAvailableFiles.length} available files`, 'success');
                    
                    mockAvailableFiles.forEach(file => {
                        const li = document.createElement('li');
                        li.className = 'file-item';
                        li.innerHTML = `
                            <div class="file-info">
                                <div class="file-name">${file.filename}</div>
                                <div class="file-size">${formatFileSize(file.size)}</div>
                                <div class="file-peers">${file.peers.length} peer(s)</div>
                            </div>
                            <button class="action-btn download-btn" 
                                    onclick="downloadFile('${file.filename}', '${file.peers[0]}')">
                                <i class="fas fa-download"></i> Download
                            </button>
                        `;
                        listElement.appendChild(li);
                    });
                } catch (error) {
                    log(`❌ Failed to refresh files: ${error.message}`, 'error');
                }
            } else {
                // Real implementation
                try {
                    log("Fetching available files from backend...");
                    
                    const backendUrl = getBackendUrl();
                    const response = await fetch(`${backendUrl}/api/files`);
                    const data = await response.json();
                    
                    const listElement = document.getElementById('availableFilesList');
                    listElement.innerHTML = '';
                    
                    if (!data.files || data.files.length === 0) {
                        listElement.innerHTML = '<li class="no-files">No files available</li>';
                        log("ℹ️ No files available on backend", 'info');
                        return;
                    }
                    
                    log(`📁 Found ${data.files.length} available files`, 'success');
                    
                    data.files.forEach(file => {
                        const li = document.createElement('li');
                        li.className = 'file-item';
                        li.innerHTML = `
                            <div class="file-info">
                                <div class="file-name">${file.filename}</div>
                                <div class="file-size">${formatFileSize(file.size)}</div>
                                <div class="file-peers">${file.peers.length} peer(s)</div>
                            </div>
                            <button class="action-btn download-btn" 
                                    onclick="downloadFile('${file.filename}', '${file.peers[0]}')">
                                <i class="fas fa-download"></i> Download
                            </button>
                        `;
                        listElement.appendChild(li);
                    });
                } catch (error) {
                    log(`❌ Failed to refresh files: ${error.message}`, 'error');
                }
            }
        }
        
        async function downloadFile(filename, peerUrl) {
            log(`Starting download: ${filename} from ${peerUrl}`);
            
            if (CONFIG.MOCK_MODE) {
                // Mock implementation
                try {
                    // Create progress element
                    const progressId = `progress-${filename.replace(/\W/g, '_')}`;
                    log(`⏳ Download progress: 
                        <div class="file-download">
                            <div>${filename}</div>
                            <div class="progress">
                                <div id="${progressId}" class="progress-bar" style="width:0%"></div>
                            </div>
                        </div>`, 'info');
                    
                    // Simulate download progress
                    for (let progress = 0; progress <= 100; progress += 10) {
                        await new Promise(resolve => setTimeout(resolve, 200));
                        const bar = document.getElementById(progressId);
                        if (bar) bar.style.width = `${progress}%`;
                    }
                    
                    log(`✅ Download complete: ${filename}`, 'success');
                } catch (error) {
                    log(`❌ Download failed: ${error.message}`, 'error');
                }
            } else {
                // Real implementation
                try {
                    // Create progress element
                    const progressId = `progress-${filename.replace(/\W/g, '_')}`;
                    log(`⏳ Download progress: 
                        <div class="file-download">
                            <div>${filename}</div>
                            <div class="progress">
                                <div id="${progressId}" class="progress-bar" style="width:0%"></div>
                            </div>
                        </div>`, 'info');
                    
                    // In a real implementation, we would use:
                    // const response = await fetch(`${peerUrl}/download/${filename}`);
                    // And track download progress
                    
                    // Simulating real download for demo
                    for (let progress = 0; progress <= 100; progress += 5) {
                        await new Promise(resolve => setTimeout(resolve, 150));
                        const bar = document.getElementById(progressId);
                        if (bar) bar.style.width = `${progress}%`;
                    }
                    
                    log(`✅ Download complete: ${filename}`, 'success');
                } catch (error) {
                    log(`❌ Download failed: ${error.message}`, 'error');
                }
            }
        }
        
        // =============================================
        // Initialization
        // =============================================
        
        function initializeUI() {
            // Populate shared files list
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
            
            // Set up mode toggle
            const modeToggle = document.getElementById('modeToggle');
            modeToggle.checked = !CONFIG.MOCK_MODE;
            modeToggle.addEventListener('change', () => {
                CONFIG.MOCK_MODE = !modeToggle.checked;
                updateConnectionStatus();
                log(`Switched to ${CONFIG.MOCK_MODE ? 'MOCK' : 'REAL'} mode`);
            });
            
            // Initialize status
            updateConnectionStatus();
            log("P2P File Sharing Client initialized", 'success');
        }
        
        // Initialize when page loads
        window.onload = initializeUI;
