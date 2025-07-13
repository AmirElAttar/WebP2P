 let requestCount = 0;
        let localConnected = false;
        let backendConnected = false;
        
        function log(message, type = 'info') {
            const output = document.getElementById('output');
            const timestamp = new Date().toLocaleTimeString();
            const logClass = `log-${type}`;
            
            const logEntry = document.createElement('div');
            logEntry.className = 'log-entry';
            logEntry.innerHTML = `
                <span class="log-timestamp">[${timestamp}]</span>
                <span class="${logClass}">${message}</span>
            `;
            
            output.appendChild(logEntry);
            output.scrollTop = output.scrollHeight;
            
            // Update last update time
            document.getElementById('lastUpdate').textContent = timestamp;
        }
        
        function updateConnectionStatus() {
            // Update local service status
            const localIndicator = document.getElementById('localStatusIndicator');
            const localStatus = document.getElementById('localConnectionStatus');
            
            if (localConnected) {
                localIndicator.className = 'status-indicator online';
                localStatus.textContent = 'Local Service: Connected';
            } else {
                localIndicator.className = 'status-indicator offline';
                localStatus.textContent = 'Local Service: Disconnected';
            }
            
            // Update backend status
            const backendIndicator = document.getElementById('backendStatusIndicator');
            const backendStatus = document.getElementById('backendConnectionStatus');
            
            if (backendConnected) {
                backendIndicator.className = 'status-indicator online';
                backendStatus.textContent = 'Backend: Connected';
            } else {
                backendIndicator.className = 'status-indicator offline';
                backendStatus.textContent = 'Backend: Disconnected';
            }
        }
        
        function getLocalServiceUrl() {
            return document.getElementById('localServiceUrl').value.trim();
        }
        
        function getBackendUrl() {
            return document.getElementById('backendUrl').value.trim();
        }
        
        async function makeRequest(url, endpoint, method = 'GET', body = null) {
            requestCount++;
            
            const fullUrl = url + endpoint;
            log(`Making ${method} request to: ${fullUrl}`, 'info');
            
            try {
                const options = {
                    method: method,
                    headers: {
                        'Content-Type': 'application/json',
                    }
                };
                
                if (body) {
                    options.body = JSON.stringify(body);
                }
                
                const response = await fetch(fullUrl, options);
                const data = await response.json();
                
                if (response.ok) {
                    log(`✅ Success: ${JSON.stringify(data, null, 2)}`, 'success');
                    return data;
                } else {
                    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                }
            } catch (error) {
                log(`❌ Error: ${error.message}`, 'error');
                throw error;
            }
        }
        
        // Test connection to local C++ peer service
        async function testLocalConnection() {
            log('🔌 Testing connection to local P2P service...', 'info');
            try {
                const data = await makeRequest(getLocalServiceUrl(), '/api/status');
                log('🎉 Local connection successful! Service is running.', 'success');
                localConnected = true;
                updateConnectionStatus();
                return data;
            } catch (error) {
                log('💥 Local connection failed. Make sure the service is running.', 'error');
                localConnected = false;
                updateConnectionStatus();
                throw error;
            }
        }
        
        // Connect to backend server
        async function connectBackend() {
            log('🌍 Connecting to backend server...', 'info');
            try {
                const data = await makeRequest(getBackendUrl(), '/api/status');
                log('🎉 Backend connection successful!', 'success');
                backendConnected = true;
                updateConnectionStatus();
                
                // Refresh available files after connecting
                await refreshAvailableFiles();
                return data;
            } catch (error) {
                log('💥 Backend connection failed.', 'error');
                backendConnected = false;
                updateConnectionStatus();
                throw error;
            }
        }
        
        // Disconnect from backend server
        function disconnectBackend() {
            log('🌍 Disconnecting from backend server...', 'info');
            backendConnected = false;
            updateConnectionStatus();
            document.getElementById('availableFilesList').innerHTML = '';
            log('Disconnected from backend server', 'success');
        }
        
        // Register shared files with backend
        async function registerSharedFiles() {
            if (!backendConnected) {
                log('⚠️ Please connect to backend first', 'warning');
                return;
            }
            
            try {
                // First get the list of shared files from local service
                const localFiles = await makeRequest(getLocalServiceUrl(), '/api/files');
                
                if (localFiles.count === 0) {
                    log('⚠️ No files to register', 'warning');
                    return;
                }
                
                // Register each file with the backend
                for (const file of localFiles.files) {
                    try {
                        const registerData = {
                            filename: file.name,
                            size: file.size,
                            peer_url: getLocalServiceUrl()
                        };
                        
                        await makeRequest(getBackendUrl(), '/api/register', 'POST', registerData);
                        log(`✅ Registered file: ${file.name} (${file.size} bytes)`, 'success');
                    } catch (error) {
                        log(`⚠️ Failed to register file: ${file.name}`, 'warning');
                    }
                }
                
                log('🎉 All files registered with backend', 'success');
            } catch (error) {
                log('💥 Failed to get shared files from local service', 'error');
            }
        }
        
        // Refresh list of available files from backend
        async function refreshAvailableFiles() {
            if (!backendConnected) {
                log('⚠️ Please connect to backend first', 'warning');
                return;
            }
            
            try {
                const data = await makeRequest(getBackendUrl(), '/api/files');
                const filesList = document.getElementById('availableFilesList');
                filesList.innerHTML = '';
                
                if (data.count === 0) {
                    const li = document.createElement('li');
                    li.textContent = 'No files available';
                    li.style.padding = '10px';
                    li.style.color = '#718096';
                    filesList.appendChild(li);
                    log('ℹ️ No files available on backend', 'info');
                    return;
                }
                
                log(`📁 Found ${data.count} available files on backend`, 'success');
                
                data.files.forEach(file => {
                    const li = document.createElement('li');
                    li.className = 'file-item';
                    
                    const fileInfo = document.createElement('div');
                    fileInfo.className = 'file-info';
                    
                    const fileName = document.createElement('div');
                    fileName.className = 'file-name';
                    fileName.textContent = file.filename;
                    
                    const fileSize = document.createElement('div');
                    fileSize.className = 'file-size';
                    fileSize.textContent = formatFileSize(file.size);
                    
                    fileInfo.appendChild(fileName);
                    fileInfo.appendChild(fileSize);
                    
                    const fileActions = document.createElement('div');
                    fileActions.className = 'file-actions';
                    
                    const downloadBtn = document.createElement('button');
                    downloadBtn.className = 'btn';
                    downloadBtn.textContent = 'Download';
                    downloadBtn.style.padding = '5px 10px';
                    downloadBtn.style.fontSize = '12px';
                    downloadBtn.onclick = () => downloadFile(file.filename, file.peer_url);
                    
                    fileActions.appendChild(downloadBtn);
                    
                    li.appendChild(fileInfo);
                    li.appendChild(fileActions);
                    
                    filesList.appendChild(li);
                });
            } catch (error) {
                log('💥 Failed to fetch available files from backend', 'error');
            }
        }
        
        // Download a file from another peer
        async function downloadFile(filename, peerUrl) {
            log(`📥 Attempting to download ${filename} from ${peerUrl}`, 'info');
            
            try {
                // First check if the peer is available
                const peerStatus = await makeRequest(peerUrl, '/api/status');
                
                // Then request the file
                const fileData = await makeRequest(peerUrl, `/api/file/${filename}`);
                
                // Here you would implement the actual download logic
                // For now we'll just log the success
                log(`✅ Successfully downloaded ${filename} (${fileData.size} bytes)`, 'success');
                
                // In a real implementation, you might:
                // 1. Save the file to disk
                // 2. Add it to your shared files
                // 3. Register it with the backend
            } catch (error) {
                log(`💥 Failed to download ${filename}: ${error.message}`, 'error');
            }
        }
        
        // Helper function to format file size
        function formatFileSize(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }
        
        // Initial load - get shared files from local service
        window.onload = async function() {
            try {
                await testLocalConnection();
                
                // Get shared files from local service
                const localFiles = await makeRequest(getLocalServiceUrl(), '/api/files');
                const sharedFilesList = document.getElementById('sharedFilesList');
                
                if (localFiles.count === 0) {
                    const li = document.createElement('li');
                    li.textContent = 'No files being shared';
                    li.style.padding = '10px';
                    li.style.color = '#718096';
                    sharedFilesList.appendChild(li);
                    log('ℹ️ No files being shared from this peer', 'info');
                    return;
                }
                
                log(`📁 Found ${localFiles.count} shared files on local service`, 'success');
                
                localFiles.files.forEach(file => {
                    const li = document.createElement('li');
                    li.className = 'file-item';
                    
                    const fileInfo = document.createElement('div');
                    fileInfo.className = 'file-info';
                    
                    const fileName = document.createElement('div');
                    fileName.className = 'file-name';
                    fileName.textContent = file.name;
                    
                    const fileSize = document.createElement('div');
                    fileSize.className = 'file-size';
                    fileSize.textContent = formatFileSize(file.size);
                    
                    fileInfo.appendChild(fileName);
                    fileInfo.appendChild(fileSize);
                    li.appendChild(fileInfo);
                    
                    sharedFilesList.appendChild(li);
                });
            } catch (error) {
                log('⚠️ Could not load shared files from local service', 'warning');
            }
        };