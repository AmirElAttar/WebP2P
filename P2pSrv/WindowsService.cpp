#include "WindowsService.h"
#include "tcpserver.h"
#include <iostream>
#include <fstream>
#include<shlobj.h>
#include "tcpclient.h"
// Static member initialization

HANDLE                CWindowsService::m_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE                CWindowsService::m_hHttpQueue = NULL;
HTTP_SERVER_SESSION_ID CWindowsService::m_SessionId = 0;
HTTP_URL_GROUP_ID     CWindowsService::m_UrlGroupId = 0;
DWORD                 CWindowsService::m_HttpPort = DEFAULT_HTTP_PORT;
std::vector<localFileHandler> CWindowsService::localFiles;
TCPFileServer* CWindowsService::m_pTCPServer;

/**
* @brief Default constructor
*/
CWindowsService::CWindowsService() {
    // Initialize TCP server pointer
}



/**
 * @brief Virtual destructor
 */
CWindowsService::~CWindowsService() {
    StopTCPServerThrd();
}
void CWindowsService::StartHttpThrd()
{
	hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	if (hThread == NULL){
        WriteToEventLog("Failed to create HTTP worker thread");
    } else {
        WriteToEventLog("HTTP worker thread created successfully");
	}
}
/**
 * @brief Start TCP server thread
 */
void CWindowsService::StartTCPServerThrd() {
    WriteToEventLog("Starting TCP server");
	
    // Create TCP server instance
	m_pTCPServer = new TCPFileServer(8080, "C:\\SharedFiles");
    
    // Initialize and start TCP server
    if (m_pTCPServer->Initialize() ) {
		m_pTCPServer->Start();
        char msg[256];
        sprintf_s(msg, "TCP server started successfully ");
        WriteToEventLog(msg);
    } else {
        WriteToEventLog("Failed to start TCP server");
        delete m_pTCPServer;
        m_pTCPServer = nullptr;
    }
}

/**
 * @brief Stop TCP server thread
 */
void CWindowsService::StopTCPServerThrd() {
    if (m_pTCPServer) {
        WriteToEventLog("Stopping TCP server");
        m_pTCPServer->Stop();
        delete m_pTCPServer;
        m_pTCPServer = nullptr;
        WriteToEventLog("TCP server stopped");
    }
}





/**
* @brief Worker thread that runs the HTTP API server
*/
DWORD WINAPI CWindowsService::ServiceWorkerThread(LPVOID lpParam){
	
	
	StartTCPServerThrd();
	
	
	WriteToEventLog("Starting HTTP API service");

	// Initialize HTTP server
	DWORD result = InitializeHttpServer();
	if (result != ERROR_SUCCESS){
		WriteToEventLog("Failed to initialize HTTP server");
		return result;
	}

	// Allocate request buffer
	PHTTP_REQUEST pRequest = (PHTTP_REQUEST)LocalAlloc(LPTR, sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE);
	if (pRequest == NULL){
		WriteToEventLog("Failed to allocate request buffer");
		CleanupHttpServer();
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	HTTP_REQUEST_ID requestId = HTTP_NULL_ID;
	DWORD bytesReceived = 0;

	WriteToEventLog("HTTP API server ready");

	// Main HTTP processing loop
	while (WaitForSingleObject(m_ServiceStopEvent, 0) != WAIT_OBJECT_0){
		ZeroMemory(pRequest, sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE);

		result = HttpReceiveHttpRequest(m_hHttpQueue, requestId, 0, pRequest,
			sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE, &bytesReceived, NULL);

		if (result == ERROR_SUCCESS){
			ProcessHttpRequest(pRequest, pRequest->RequestId);
			requestId = HTTP_NULL_ID;
		}else if (result == ERROR_MORE_DATA){
			SendJsonResponse(pRequest->RequestId, 413, "{\"error\":\"Request too large\"}");
			requestId = HTTP_NULL_ID;
		}else if (result != WAIT_TIMEOUT && result != ERROR_OPERATION_ABORTED){
			Sleep(100);
		}

		if (WaitForSingleObject(m_ServiceStopEvent, 10) == WAIT_OBJECT_0)
			break;
	}

	// Cleanup
	LocalFree(pRequest);
	CleanupHttpServer();
	WriteToEventLog("HTTP API service stopped");
	return ERROR_SUCCESS;
}



/**
* @brief Initialize HTTP server components
*/
DWORD CWindowsService::InitializeHttpServer(){
	DWORD result = ERROR_SUCCESS;
	HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;

	WriteToEventLog("Initializing HTTP API server");

	// Initialize HTTP Server API
	result = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
	if (result != NO_ERROR){
		WriteToEventLog("HttpInitialize failed");
		return result;
	}

	// Create server session
	result = HttpCreateServerSession(httpApiVersion, &m_SessionId, 0);
	if (result != NO_ERROR){
		WriteToEventLog("HttpCreateServerSession failed");
		HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return result;
	}

	// Create URL group
	result = HttpCreateUrlGroup(m_SessionId, &m_UrlGroupId, 0);
	if (result != NO_ERROR){
		WriteToEventLog("HttpCreateUrlGroup failed");
		HttpCloseServerSession(m_SessionId);
		HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return result;
	}

	// Create request queue
	result = HttpCreateRequestQueue(httpApiVersion, L"P2pApiQueue", NULL, 0, &m_hHttpQueue);
	if (result != NO_ERROR){
		WriteToEventLog("HttpCreateRequestQueue failed");
		HttpCloseUrlGroup(m_UrlGroupId);
		HttpCloseServerSession(m_SessionId);
		HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return result;
	}

	// Get configured port
	m_HttpPort = GetHttpPortFromRegistry();

	// Build URL to listen on
	WCHAR wszUrl[256];
	swprintf_s(wszUrl, 256, HTTP_URL_PREFIX, m_HttpPort);

	// Add URL to group
	result = HttpAddUrlToUrlGroup(m_UrlGroupId, wszUrl, 0, 0);
	if (result != NO_ERROR){
		WriteToEventLog("HttpAddUrlToUrlGroup failed");
		CleanupHttpServer();
		return result;
	}

	// Bind request queue to URL group
	HTTP_BINDING_INFO bindingInfo;
	bindingInfo.Flags.Present = 1;
	bindingInfo.RequestQueueHandle = m_hHttpQueue;

	result = HttpSetUrlGroupProperty(m_UrlGroupId, HttpServerBindingProperty,
		&bindingInfo, sizeof(bindingInfo));
	if (result != NO_ERROR){
		WriteToEventLog("HttpSetUrlGroupProperty failed");
		CleanupHttpServer();
		return result;
	}

	char szMessage[256];
	sprintf(szMessage,  "HTTP API server listening on port %d", m_HttpPort);
	WriteToEventLog(szMessage);

	return ERROR_SUCCESS;
}

/**
* @brief Clean up HTTP server resources
*/
void CWindowsService::CleanupHttpServer(){
	WriteToEventLog("Cleaning up HTTP server");

	if (m_hHttpQueue != NULL){
		HttpCloseRequestQueue(m_hHttpQueue);
		m_hHttpQueue = NULL;
	}

	if (m_UrlGroupId != 0){
		HttpCloseUrlGroup(m_UrlGroupId);
		m_UrlGroupId = 0;
	}

	if (m_SessionId != 0){
		HttpCloseServerSession(m_SessionId);
		m_SessionId = 0;
	}

	HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
}

/**
* @brief Process incoming HTTP requests
*/
DWORD CWindowsService::ProcessHttpRequest(PHTTP_REQUEST pRequest, HTTP_REQUEST_ID RequestId){
	// Extract URL path
	std::string url;
	if (pRequest->CookedUrl.pAbsPath){
		int len = WideCharToMultiByte(CP_UTF8, 0, pRequest->CookedUrl.pAbsPath, -1, NULL, 0, NULL, NULL);
		if (len > 0){
			url.resize(len - 1);
			WideCharToMultiByte(CP_UTF8, 0, pRequest->CookedUrl.pAbsPath, -1, &url[0], len, NULL, NULL);
		}
	}

	// Get HTTP method
	std::string method;
	switch (pRequest->Verb){
	case HttpVerbGET:    method = "GET"; break;
	case HttpVerbPOST:   method = "POST"; break;
	case HttpVerbPUT:    method = "PUT"; break;
	case HttpVerbDELETE: method = "DELETE"; break;
	case HttpVerbOPTIONS: method = "OPTIONS"; break;
	default:             method = "UNKNOWN"; break;
	}

 // Extract request body for POST requests
    std::string requestBody;
    if (method == "POST" && pRequest->EntityChunkCount > 0) {
        PHTTP_DATA_CHUNK pDataChunk = &pRequest->pEntityChunks[0];
        if (pDataChunk->DataChunkType == HttpDataChunkFromMemory) {
            requestBody.assign((char*)pDataChunk->FromMemory.pBuffer, pDataChunk->FromMemory.BufferLength);
        }
    }
	// Log API request
	char szLog[256];
	sprintf(szLog,  "API: %S %S", method.c_str(), url.c_str());
	WriteToEventLog(szLog);

	// Handle CORS preflight
	if (method == "OPTIONS"){
		return SendJsonResponse(RequestId, 200, "");
	}

	// Route API requests
	if (url.find("/api/") == 0){
        std::string response = HandleApiRequest(url.c_str(), method.c_str(), requestBody.c_str());
		return SendJsonResponse(RequestId, 200, response.c_str());
	}else{
		// Non-API requests get 404
		return SendJsonResponse(RequestId, 404, "{\"error\":\"API endpoint not found\"}");
	}
}

/**
* @brief Send JSON response with CORS headers
*/
DWORD CWindowsService::SendJsonResponse(HTTP_REQUEST_ID RequestId, USHORT StatusCode, const char* pJsonContent){
	HTTP_RESPONSE response;
	HTTP_DATA_CHUNK dataChunk;

	// Initialize response
	ZeroMemory(&response, sizeof(response));
	response.StatusCode = StatusCode;
	response.pReason = (StatusCode == 200) ? "OK" : (StatusCode == 404) ? "Not Found" : "Error";
	response.ReasonLength = (USHORT)strlen(response.pReason);

	// Set JSON content type
	response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "application/json";
	response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = 16;

	// Add CORS headers
	HTTP_UNKNOWN_HEADER corsHeaders[3];
	corsHeaders[0].pName = "Access-Control-Allow-Origin";
	corsHeaders[0].NameLength = 27;
	corsHeaders[0].pRawValue = "*";
	corsHeaders[0].RawValueLength = 1;

	corsHeaders[1].pName = "Access-Control-Allow-Methods";
	corsHeaders[1].NameLength = 28;
	corsHeaders[1].pRawValue = "GET, POST, PUT, DELETE, OPTIONS";
	corsHeaders[1].RawValueLength = 31;

	corsHeaders[2].pName = "Access-Control-Allow-Headers";
	corsHeaders[2].NameLength = 28;
	corsHeaders[2].pRawValue = "Content-Type, Authorization";
	corsHeaders[2].RawValueLength = 27;

	response.Headers.UnknownHeaderCount = 3;
	response.Headers.pUnknownHeaders = corsHeaders;

	// Set response body
	if (pJsonContent && strlen(pJsonContent) > 0){
		dataChunk.DataChunkType = HttpDataChunkFromMemory;
		dataChunk.FromMemory.pBuffer = (PVOID)pJsonContent;
		dataChunk.FromMemory.BufferLength = (ULONG)strlen(pJsonContent);
		response.EntityChunkCount = 1;
		response.pEntityChunks = &dataChunk;
	}

	return HttpSendHttpResponse(m_hHttpQueue, RequestId, 0, &response, NULL, NULL, NULL, 0, NULL, NULL);
}

/**
* @brief Handle API endpoint requests
*/
std::string CWindowsService::HandleApiRequest(const char* pPath, const char* pMethod, const char* pRequestBody) {
	std::ostringstream json;

	if (strcmp(pPath, "/api/status") == 0 && strcmp(pMethod, "GET") == 0){
		json << "{\"status\":\"running\",\"port\":" << m_HttpPort << ",\"uptime\":" << (GetTickCount() / 1000) << "}";
	}else if (strcmp(pPath, "/api/files") == 0 && strcmp(pMethod, "GET") == 0){
		std::string folder=ShowFolderSelection();
		if (folder != ""){
			enumerateFiles(folder);
			json << "{\"files\":[";
			for (size_t i = 0; i < localFiles.size(); ++i) {
				 auto& file = localFiles[i];
				json << "{";
				json << "\"filename\":\"" << file.getshortName() << "\",";
				json << "\"size\":" << file.getFileSize() << ",";
				json << "\"sha256\":\"" << file.getHash() << "\",";
				json << "\"creation\":\"" << file.getCreationDate() << "\",";
				json << "\"modified\":\"" << file.getLastWriteDate() << "\"";
				json << "}";
				if (i != localFiles.size() - 1)
					json << ",";
			}
			json << "],\"count\":" << localFiles.size() << "}";
		}else{
			WriteToEventLog("Returning empty file list");
			json << "{\"files\":[";
			json << "],\"count\":" << "0, files not found" << "}";
		}
		/*json << "{\"files\":[";
		json << "{\"name\":\"document.pdf\",\"size\":1024000,\"path\":\"C:\\\\Shared\\\\document.pdf\"},";
		json << "{\"name\":\"image.jpg\",\"size\":512000,\"path\":\"C:\\\\Shared\\\\image.jpg\"}";
		json << "],\"count\":2}";*/
		//new code to write the real data
		

	}
	else if (strncmp(pPath, "/api/file/", 10) == 0 && strcmp(pMethod, "GET") == 0){
		const char* filename = pPath + 10;
		json << "{\"filename\":\"" << filename << "\",\"data\":\"base64data\",\"size\":1024}";
	}
	else if (strcmp(pPath, "/api/upload") == 0 && strcmp(pMethod, "POST") == 0){
		json << "{\"success\":true,\"message\":\"File uploaded successfully\"}";
	}
    else if (strcmp(pPath, "/api/download") == 0 && strcmp(pMethod, "POST") == 0) {
        // Handle file download request using TCP client
        json << HandleDownloadRequest(pRequestBody);
    }
    else if (strcmp(pPath, "/api/peers") == 0 && strcmp(pMethod, "GET") == 0) {
        json << "{\"peers\":[{\"id\":\"peer1\",\"ip\":\"192.168.1.100\",\"port\":8847}],\"count\":1}";
    }
    else {
		json << "{\"error\":\"Unknown API endpoint\",\"path\":\"" << pPath << "\"}";
	}

	return json.str();
}

/**
 * @brief Handle download request from web interface
 */
std::string CWindowsService::HandleDownloadRequest(const char* pRequestBody) {
    if (!pRequestBody) {
        return "{\"success\":false,\"message\":\"No request body provided\"}";
    }
    
    std::string requestJson(pRequestBody);
    
    // Extract filename and IP addresses from JSON
    std::string filename = ExtractJsonValue(requestJson, "filename");
    std::string firstIP = ExtractFirstIP(requestJson, "ip_addresses");
    
    if (filename.empty() || firstIP.empty()) {
        return "{\"success\":false,\"message\":\"Missing filename or IP addresses\"}";
    }
    
    // Log the download request
    char logMsg[512];
    sprintf_s(logMsg, "Download request: %s from %s", filename.c_str(), firstIP.c_str());
    WriteToEventLog(logMsg);
    
    // Create output path
    std::string outputPath = "C:\\Downloads\\" + filename;
    
    // Use TCP client to download file
    bool downloadSuccess = DownloadFileFromPeer(firstIP, filename, outputPath);
    
    if (downloadSuccess) {
        return "{\"success\":true,\"message\":\"File downloaded successfully\",\"filename\":\"" + filename + "\",\"source_ip\":\"" + firstIP + "\"}";
    } else {
        return "{\"success\":false,\"message\":\"Download failed\",\"filename\":\"" + filename + "\",\"source_ip\":\"" + firstIP + "\"}";
    }
}
/**
 * @brief Download file from peer using TCP client
 */
bool CWindowsService::DownloadFileFromPeer(const std::string& serverIP, const std::string& filename, const std::string& outputPath) {
    // Create TCP client instance from your client.h
	TCPFileClient client("", 0);
    
    // Initialize client
    if (!client.Initialize()) {
        WriteToEventLog("Failed to initialize TCP client");
        return false;
    }
    
    // Download file using the client
    bool result = client.DownloadFileFromServer(serverIP, filename, outputPath);
    
    if (result) {
        char logMsg[512];
        sprintf_s(logMsg, "Successfully downloaded %s from %s", filename.c_str(), serverIP.c_str());
        WriteToEventLog(logMsg);
    } else {
        char logMsg[512];
        sprintf_s(logMsg, "Failed to download %s from %s", filename.c_str(), serverIP.c_str());
        WriteToEventLog(logMsg);
    }
    
    return result;
}
/**
 * @brief Extract JSON value by key
 */
std::string CWindowsService::ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    size_t valueStart = json.find('"', colonPos);
    if (valueStart == std::string::npos) {
        return "";
    }
    valueStart++; // Skip opening quote
    
    size_t valueEnd = json.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return "";
    }
    
    return json.substr(valueStart, valueEnd - valueStart);
}
/**
 * @brief Extract first IP address from JSON array
 */
std::string CWindowsService::ExtractFirstIP(const std::string& json, const std::string& arrayKey) {
    std::string searchKey = "\"" + arrayKey + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    size_t arrayStart = json.find('[', keyPos);
    if (arrayStart == std::string::npos) {
        return "";
    }
    
    size_t firstQuote = json.find('"', arrayStart);
    if (firstQuote == std::string::npos) {
        return "";
    }
    firstQuote++; // Skip opening quote
    
    size_t secondQuote = json.find('"', firstQuote);
    if (secondQuote == std::string::npos) {
        return "";
    }
    
    return json.substr(firstQuote, secondQuote - firstQuote);
}
/*brief Get HTTP port from registry configuration
*/
DWORD CWindowsService::GetHttpPortFromRegistry(){
	HKEY hKey;
	DWORD dwPort = DEFAULT_HTTP_PORT;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SYSTEM\\CurrentControlSet\\Services\\P2pWindowsService\\Parameters"),
		0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(DWORD);
		RegQueryValueEx(hKey, _T("HttpPort"), NULL, NULL, (LPBYTE)&dwPort, &dwSize);
		if (dwPort < 1024 || dwPort > 65535)
			dwPort = DEFAULT_HTTP_PORT;
		RegCloseKey(hKey);
	}

	return dwPort;
}

std::string CWindowsService::ShowFolderSelection()
{
	std::string result = "";
		// Initialize COM
		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr)) 
		{
			WriteToEventLog("COM initialization failed.");
			return result;
		}

		BROWSEINFOA bi = { 0 };
		bi.lpszTitle = "Select folder to share with peers";
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;

		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl != NULL) 
		{
			char path[MAX_PATH];
			if (SHGetPathFromIDListA(pidl, path)) 
			{
				result = path;
			}
			else 
			{
				WriteToEventLog("Failed to get folder path.");
			}
			CoTaskMemFree(pidl);
		}
		else 
		{
			WriteToEventLog("No folder selected.");
		}

		CoUninitialize();
		return result;
}







/**
* @brief Write message to Windows Event Log
*/
void CWindowsService::WriteToEventLog(char* pszMessage)
{

	static char logFilePath[MAX_PATH] = { 0 };

	// Build the log file path once
	if (logFilePath[0] == 0) 
	{
		char tempPath[MAX_PATH];
		if (GetTempPathA(MAX_PATH, tempPath)) 
		{
			StringCchPrintfA(logFilePath, MAX_PATH, "%s%s", tempPath, "MyServiceApp.log");
		}
		else 
		{
			return; // Failed to get temp path
		}
	}


	// Open the log file in append mode
	HANDLE hFile = CreateFileA(
		logFilePath,
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	if (hFile != INVALID_HANDLE_VALUE) 
	{
		DWORD written;
		SYSTEMTIME st;
		GetLocalTime(&st);

		char buffer[1024];
		StringCchPrintfA(buffer, 1024, "[%04d-%02d-%02d %02d:%02d:%02d] %s\r\n",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond,
			pszMessage);

		WriteFile(hFile, buffer, lstrlenA(buffer), &written, NULL);
		CloseHandle(hFile);
	}


}

void CWindowsService::enumerateFiles(std::string folderPath)
{
	//clear local files array
	localFiles.clear();
	// Make search pattern: folder\*
	std::string searchPattern = folderPath;
	if (!searchPattern.empty() && searchPattern.back() != '\\')
		searchPattern += '\\';
	searchPattern += "*";

	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		WriteToEventLog("FindFirstFileA failed , empty folder or system error");
		return ; // No files found or error
	}
	do
	{
		// Skip "." and ".."
		if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			continue;

		// Only include normal files (not directories)
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		// skip hidden/system files:
		 if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
		    continue;

		// Full path
		std::string fullPath = folderPath;
		if (!fullPath.empty() && fullPath.back() != '\\')
			fullPath += '\\';
		fullPath += findData.cFileName;
		localFileHandler f(fullPath);
		f.setfileSize(findData.nFileSizeLow, findData.nFileSizeHigh);
		f.setCreationDate(findData.ftCreationTime);
		f.setWriteTime(findData.ftLastWriteTime);
		localFiles.push_back(f);

	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
	
}