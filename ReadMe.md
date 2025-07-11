# P2P Windows Service - Complete Usage Guide

Congratulations on successfully building your P2P Windows Service with HTTP API! Here's a comprehensive markdown documentation for your milestone.

## README.md

```markdown
# P2P Windows Service with HTTP API

A Windows service that provides a minimal HTTP API server for P2P file sharing operations. The service handles JSON API requests without generating HTML content, making it perfect for integration with web-based clients.

## Features

- **Windows Service Integration** - Runs as a background Windows service
- **HTTP API Server** - Lightweight HTTP server using Windows HTTP.sys
- **JSON-only Responses** - Clean API responses for web integration
- **CORS Support** - Cross-origin requests enabled for web clients
- **Event Logging** - Comprehensive logging to Windows Event Log
- **Registry Configuration** - Configurable HTTP port via Windows Registry

## API Endpoints

- `GET /api/status` - Service status and uptime information
- `GET /api/files` - List available files for sharing
- `GET /api/file/{filename}` - Get specific file data
- `POST /api/upload` - Upload file to service
- `GET /api/peers` - List connected peers

## Prerequisites

- **Visual Studio 2013** or later
- **Windows SDK 8.1** or later
- **Administrator privileges** for service installation
- **Windows 7/Server 2008 R2** or later

## Building the Service

### 1. Project Setup

1. **Open Visual Studio 2013** as Administrator
2. **Create new project**:
   - File → New → Project
   - Visual C++ → Win32 Console Application
   - Project Name: `P2pSrv` (or your preferred name)
   - Choose "Empty project"

### 2. Add Source Files

Add these files to your project:

- `WindowsService.h` - Header file with class declarations
- `WindowsService.cpp` - Main service implementation
- `main.cpp` - Application entry point

### 3. Project Configuration

**Build Configuration:**
- Configuration: **Release** (recommended for services)
- Platform: **Win32** or **x64**
- Subsystem: **Console** (do not change to Windows)

**Required Libraries:**
The following libraries are automatically linked via pragma directives:
- `httpapi.lib` - HTTP Server API
- `ws2_32.lib` - Winsock2

### 4. Build Process

1. **Set build configuration** to Release
2. **Build solution**:
   ```
   Build → Rebuild Solution
   ```
3. **Verify successful build** - check for 0 errors
4. **Locate executable** in `Release` folder

## Installing and Running the Service

### 1. Service Installation

**Run Command Prompt as Administrator:**

```
cd [path-to-your-release-folder]
YourServiceName.exe install
```

**Expected output:**
```
Service installed successfully
```

### 2. Starting the Service

**Method 1: Command Line**
```
sc start P2pWindowsService
```

**Method 2: Services Management Console**
1. Press `Win + R`, type `services.msc`, press Enter
2. Find "P2p file sharing Service"
3. Right-click → Start

**Method 3: NET Command**
```
net start P2pWindowsService
```

### 3. Verify Service is Running

**Check service status:**
```
sc query P2pWindowsService
```

**Expected output:**
```
SERVICE_NAME: P2pWindowsService
TYPE               : 10  WIN32_OWN_PROCESS
STATE              : 4  RUNNING
...
```

**Check HTTP server:**
```
netstat -an | findstr 8847
```

**Expected output:**
```
TCP    0.0.0.0:8847           0.0.0.0:0              LISTENING
```

### 4. Monitor Service Logs

1. **Open Event Viewer** (`eventvwr.msc`)
2. **Navigate to**: Windows Logs → Application
3. **Filter by source**: P2pWindowsService
4. **Look for messages**:
   - "Starting HTTP API service"
   - "HTTP API server listening on port 8847"
   - "HTTP API server ready"

## Uninstalling the Service

### 1. Stop the Service

**Always stop the service before uninstalling:**

```
sc stop P2pWindowsService
```

**Or:**
```
net stop P2pWindowsService
```

### 2. Uninstall the Service

**Method 1: Using your executable**
```
YourServiceName.exe uninstall
```

**Method 2: Using SC command**
```
sc delete P2pWindowsService
```

**Expected output:**
```
Service deleted successfully
```

### 3. Verify Removal

```
sc query P2pWindowsService
```

**Expected output:**
```
[SC] EnumQueryServicesStatus:OpenService FAILED 1060:
The specified service does not exist as an installed service.
```

## Testing with HTML Test Client

### 1. Setup Test Environment

1. **Save the test client** as `test-client.html`
2. **Ensure service is running** (see installation steps above)
3. **Open HTML file** in a web browser (Chrome, Firefox, Edge)

### 2. Basic Connectivity Test

1. **Open test-client.html** in your browser
2. **Verify default URL** is `http://localhost:8847`
3. **Click "Test Connection"** button
4. **Expected results**:
   - Status indicator turns green
   - "Connection successful!" message appears
   - Service statistics populate (Port, Uptime)

### 3. API Endpoint Testing

#### Status Endpoint Test
1. **Click "Get Status"**
2. **Expected response**:
   ```
   {
     "status": "running",
     "port": 8847,
     "uptime": 123
   }
   ```

#### Files Endpoint Test
1. **Click "List Files"**
2. **Expected response**:
   ```
   {
     "files": [
       {
         "name": "document.pdf",
         "size": 1024000,
         "path": "C:\\Shared\\document.pdf"
       },
       {
         "name": "image.jpg",
         "size": 512000,
         "path": "C:\\Shared\\image.jpg"
       }
     ],
     "count": 2
   }
   ```

#### Peers Endpoint Test
1. **Click "Get Peers"**
2. **Expected response**:
   ```
   {
     "peers": [
       {
         "id": "peer1",
         "ip": "192.168.1.100",
         "port": 8847
       }
     ],
     "count": 1
   }
   ```

#### File Download Test
1. **Enter filename** in "Test Filename" field (e.g., "test.txt")
2. **Click "Get File"**
3. **Expected response**:
   ```
   {
     "filename": "test.txt",
     "data": "base64data",
     "size": 1024
   }
   ```

#### Upload Test
1. **Click "Upload File"**
2. **Expected response**:
   ```
   {
     "success": true,
     "message": "File uploaded successfully"
   }
   ```

### 4. Advanced Testing

#### Stress Test
1. **Click "Stress Test"** to send 10 rapid requests
2. **Monitor results**:
   - All requests should succeed
   - Average response time should be < 100ms
   - No errors should occur

#### Error Handling Test
1. **Stop the service**: `sc stop P2pWindowsService`
2. **Click "Test Connection"**
3. **Expected result**: Connection failed message
4. **Restart service**: `sc start P2pWindowsService`
5. **Test connection again**: Should succeed

#### CORS Verification
1. **Open browser Developer Tools** (F12)
2. **Go to Network tab**
3. **Make any API request**
4. **Check response headers** for:
   - `Access-Control-Allow-Origin: *`
   - `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`

### 5. Test Results Interpretation

#### Success Indicators
- ✅ Status indicator shows green (online)
- ✅ All API endpoints return valid JSON
- ✅ Response times under 100ms
- ✅ No errors in browser console
- ✅ Service logs show API requests in Event Viewer

#### Common Issues and Solutions

**Connection Refused:**
- Verify service is running: `sc query P2pWindowsService`
- Check Windows Firewall settings
- Ensure port 8847 is not blocked

**HTTP 404 Errors:**
- Verify URL paths start with `/api/`
- Check service initialization in Event Viewer
- Restart service if needed

**CORS Errors:**
- Access HTML file via `http://localhost` instead of `file://`
- Check browser console for specific CORS messages
- Verify service is sending proper headers

## Configuration

### Custom HTTP Port

To change the default port (8847):

1. **Open Registry Editor** (`regedit.exe`) as Administrator
2. **Navigate to**:
   ```
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\P2pWindowsService\Parameters
   ```
3. **Create DWORD value**:
   - Name: `HttpPort`
   - Value: Your desired port (e.g., 9000)
4. **Restart the service**:
   ```
   sc stop P2pWindowsService
   sc start P2pWindowsService
   ```

### Service Startup Type

**Set to automatic startup:**
```
sc config P2pWindowsService start= auto
```

**Set to manual startup:**
```
sc config P2pWindowsService start= demand
```

## Architecture

### Service Components
- **Service Control Manager Integration** - Standard Windows service lifecycle
- **HTTP Server** - Uses Windows HTTP.sys for production-grade HTTP handling
- **API Router** - Routes requests to appropriate handlers
- **Event Logging** - Comprehensive logging for monitoring and debugging

### Security Features
- **HTTP.sys Integration** - Kernel-mode HTTP processing
- **CORS Headers** - Controlled cross-origin access
- **Input Validation** - Request size limits and validation
- **Error Handling** - Graceful error responses without information disclosure

## Development Notes

### Visual Studio 2013 Compatibility
- Uses C++11 features available in VS2013
- Compatible with Windows SDK 8.1
- No external dependencies required

### Extension Points
- Add new API endpoints in `HandleApiRequest()`
- Implement file system operations
- Add peer discovery functionality
- Integrate with P2P networking protocols

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]
```

## Commit Message Suggestion

```
feat: Complete P2P Windows Service with HTTP API

- Implemented minimal HTTP API server using Windows HTTP.sys
- Added JSON-only endpoints for status, files, peers, and upload
- Integrated Windows Service lifecycle management
- Added CORS support for web client integration
- Comprehensive event logging to Windows Event Log
- Registry-based port configuration
- Complete HTML test client for API validation
- Full documentation with build, install, and test instructions

Endpoints:
- GET /api/status - Service status and uptime
- GET /api/files - List available files
- GET /api/file/{filename} - Get file data
- POST /api/upload - Upload files
- GET /api/peers - List connected peers

Tested on Windows with Visual Studio 2013
Ready for P2P file sharing integration
```
