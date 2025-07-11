#include "WindowsService.h"

// Static member initialization
SERVICE_STATUS        CWindowsService::m_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE CWindowsService::m_StatusHandle = NULL;
HANDLE                CWindowsService::m_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE                CWindowsService::m_hHttpQueue = NULL;
HTTP_SERVER_SESSION_ID CWindowsService::m_SessionId = 0;
HTTP_URL_GROUP_ID     CWindowsService::m_UrlGroupId = 0;
DWORD                 CWindowsService::m_HttpPort = DEFAULT_HTTP_PORT;

/**
* @brief Default constructor
*/
CWindowsService::CWindowsService()
{
}

/**
* @brief Virtual destructor
*/
CWindowsService::~CWindowsService()
{
}

/**
* @brief Main service entry point called by SCM
*/
void WINAPI CWindowsService::ServiceMain(DWORD argc, LPTSTR *argv)
{
	// Register service control handler
	m_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (m_StatusHandle == NULL)
	{
		WriteToEventLog(_T("RegisterServiceCtrlHandler failed"), EVENTLOG_ERROR_TYPE);
		return;
	}

	// Initialize service status
	ZeroMemory(&m_ServiceStatus, sizeof(m_ServiceStatus));
	m_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	m_ServiceStatus.dwWin32ExitCode = 0;
	m_ServiceStatus.dwServiceSpecificExitCode = 0;
	m_ServiceStatus.dwCheckPoint = 0;

	// Report initial status to SCM
	SetServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Create stop event
	m_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_ServiceStopEvent == NULL)
	{
		WriteToEventLog(_T("CreateEvent failed"), EVENTLOG_ERROR_TYPE);
		SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status
	SetServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// Create worker thread
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	if (hThread != NULL)
	{
		// Wait for stop event
		WaitForSingleObject(m_ServiceStopEvent, INFINITE);
		CloseHandle(hThread);
	}

	// Cleanup
	CloseHandle(m_ServiceStopEvent);
	SetServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

/**
* @brief Service control handler for SCM commands
*/
void WINAPI CWindowsService::ServiceCtrlHandler(DWORD ctrl)
{
	switch (ctrl)
	{
	case SERVICE_CONTROL_STOP:
		SetServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		SetEvent(m_ServiceStopEvent);
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

/**
* @brief Worker thread that runs the HTTP API server
*/
DWORD WINAPI CWindowsService::ServiceWorkerThread(LPVOID lpParam)
{
	WriteToEventLog(_T("Starting HTTP API service"), EVENTLOG_INFORMATION_TYPE);

	// Initialize HTTP server
	DWORD result = InitializeHttpServer();
	if (result != ERROR_SUCCESS)
	{
		WriteToEventLog(_T("Failed to initialize HTTP server"), EVENTLOG_ERROR_TYPE);
		return result;
	}

	// Allocate request buffer
	PHTTP_REQUEST pRequest = (PHTTP_REQUEST)LocalAlloc(LPTR, sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE);
	if (pRequest == NULL)
	{
		WriteToEventLog(_T("Failed to allocate request buffer"), EVENTLOG_ERROR_TYPE);
		CleanupHttpServer();
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	HTTP_REQUEST_ID requestId = HTTP_NULL_ID;
	DWORD bytesReceived = 0;

	WriteToEventLog(_T("HTTP API server ready"), EVENTLOG_INFORMATION_TYPE);

	// Main HTTP processing loop
	while (WaitForSingleObject(m_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		ZeroMemory(pRequest, sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE);

		result = HttpReceiveHttpRequest(m_hHttpQueue, requestId, 0, pRequest,
			sizeof(HTTP_REQUEST) + MAX_REQUEST_SIZE, &bytesReceived, NULL);

		if (result == ERROR_SUCCESS)
		{
			ProcessHttpRequest(pRequest, pRequest->RequestId);
			requestId = HTTP_NULL_ID;
		}
		else if (result == ERROR_MORE_DATA)
		{
			SendJsonResponse(pRequest->RequestId, 413, "{\"error\":\"Request too large\"}");
			requestId = HTTP_NULL_ID;
		}
		else if (result != WAIT_TIMEOUT && result != ERROR_OPERATION_ABORTED)
		{
			Sleep(100);
		}

		if (WaitForSingleObject(m_ServiceStopEvent, 10) == WAIT_OBJECT_0)
			break;
	}

	// Cleanup
	LocalFree(pRequest);
	CleanupHttpServer();
	WriteToEventLog(_T("HTTP API service stopped"), EVENTLOG_INFORMATION_TYPE);
	return ERROR_SUCCESS;
}

/**
* @brief Start service programmatically (placeholder)
*/
void CWindowsService::Start()
{
	// Implementation placeholder
}

/**
* @brief Stop service programmatically (placeholder)
*/
void CWindowsService::Stop()
{
	// Implementation placeholder
}

/**
* @brief Initialize HTTP server components
*/
DWORD CWindowsService::InitializeHttpServer()
{
	DWORD result = ERROR_SUCCESS;
	HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_2;

	WriteToEventLog(_T("Initializing HTTP API server"), EVENTLOG_INFORMATION_TYPE);

	// Initialize HTTP Server API
	result = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpInitialize failed"), EVENTLOG_ERROR_TYPE);
		return result;
	}

	// Create server session
	result = HttpCreateServerSession(httpApiVersion, &m_SessionId, 0);
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpCreateServerSession failed"), EVENTLOG_ERROR_TYPE);
		HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return result;
	}

	// Create URL group
	result = HttpCreateUrlGroup(m_SessionId, &m_UrlGroupId, 0);
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpCreateUrlGroup failed"), EVENTLOG_ERROR_TYPE);
		HttpCloseServerSession(m_SessionId);
		HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
		return result;
	}

	// Create request queue
	result = HttpCreateRequestQueue(httpApiVersion, L"P2pApiQueue", NULL, 0, &m_hHttpQueue);
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpCreateRequestQueue failed"), EVENTLOG_ERROR_TYPE);
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
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpAddUrlToUrlGroup failed"), EVENTLOG_ERROR_TYPE);
		CleanupHttpServer();
		return result;
	}

	// Bind request queue to URL group
	HTTP_BINDING_INFO bindingInfo;
	bindingInfo.Flags.Present = 1;
	bindingInfo.RequestQueueHandle = m_hHttpQueue;

	result = HttpSetUrlGroupProperty(m_UrlGroupId, HttpServerBindingProperty,
		&bindingInfo, sizeof(bindingInfo));
	if (result != NO_ERROR)
	{
		WriteToEventLog(_T("HttpSetUrlGroupProperty failed"), EVENTLOG_ERROR_TYPE);
		CleanupHttpServer();
		return result;
	}

	TCHAR szMessage[256];
	_stprintf_s(szMessage, 256, _T("HTTP API server listening on port %d"), m_HttpPort);
	WriteToEventLog(szMessage, EVENTLOG_INFORMATION_TYPE);

	return ERROR_SUCCESS;
}

/**
* @brief Clean up HTTP server resources
*/
void CWindowsService::CleanupHttpServer()
{
	WriteToEventLog(_T("Cleaning up HTTP server"), EVENTLOG_INFORMATION_TYPE);

	if (m_hHttpQueue != NULL)
	{
		HttpCloseRequestQueue(m_hHttpQueue);
		m_hHttpQueue = NULL;
	}

	if (m_UrlGroupId != 0)
	{
		HttpCloseUrlGroup(m_UrlGroupId);
		m_UrlGroupId = 0;
	}

	if (m_SessionId != 0)
	{
		HttpCloseServerSession(m_SessionId);
		m_SessionId = 0;
	}

	HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
}

/**
* @brief Process incoming HTTP requests
*/
DWORD CWindowsService::ProcessHttpRequest(PHTTP_REQUEST pRequest, HTTP_REQUEST_ID RequestId)
{
	// Extract URL path
	std::string url;
	if (pRequest->CookedUrl.pAbsPath)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, pRequest->CookedUrl.pAbsPath, -1, NULL, 0, NULL, NULL);
		if (len > 0)
		{
			url.resize(len - 1);
			WideCharToMultiByte(CP_UTF8, 0, pRequest->CookedUrl.pAbsPath, -1, &url[0], len, NULL, NULL);
		}
	}

	// Get HTTP method
	std::string method;
	switch (pRequest->Verb)
	{
	case HttpVerbGET:    method = "GET"; break;
	case HttpVerbPOST:   method = "POST"; break;
	case HttpVerbPUT:    method = "PUT"; break;
	case HttpVerbDELETE: method = "DELETE"; break;
	case HttpVerbOPTIONS: method = "OPTIONS"; break;
	default:             method = "UNKNOWN"; break;
	}

	// Log API request
	TCHAR szLog[256];
	_stprintf_s(szLog, 256, _T("API: %S %S"), method.c_str(), url.c_str());
	WriteToEventLog(szLog, EVENTLOG_INFORMATION_TYPE);

	// Handle CORS preflight
	if (method == "OPTIONS")
	{
		return SendJsonResponse(RequestId, 200, "");
	}

	// Route API requests
	if (url.find("/api/") == 0)
	{
		std::string response = HandleApiRequest(url.c_str(), method.c_str());
		return SendJsonResponse(RequestId, 200, response.c_str());
	}
	else
	{
		// Non-API requests get 404
		return SendJsonResponse(RequestId, 404, "{\"error\":\"API endpoint not found\"}");
	}
}

/**
* @brief Send JSON response with CORS headers
*/
DWORD CWindowsService::SendJsonResponse(HTTP_REQUEST_ID RequestId, USHORT StatusCode, const char* pJsonContent)
{
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
	if (pJsonContent && strlen(pJsonContent) > 0)
	{
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
std::string CWindowsService::HandleApiRequest(const char* pPath, const char* pMethod)
{
	std::ostringstream json;

	if (strcmp(pPath, "/api/status") == 0 && strcmp(pMethod, "GET") == 0)
	{
		json << "{\"status\":\"running\",\"port\":" << m_HttpPort << ",\"uptime\":" << (GetTickCount() / 1000) << "}";
	}
	else if (strcmp(pPath, "/api/files") == 0 && strcmp(pMethod, "GET") == 0)
	{
		json << "{\"files\":[";
		json << "{\"name\":\"document.pdf\",\"size\":1024000,\"path\":\"C:\\\\Shared\\\\document.pdf\"},";
		json << "{\"name\":\"image.jpg\",\"size\":512000,\"path\":\"C:\\\\Shared\\\\image.jpg\"}";
		json << "],\"count\":2}";
	}
	else if (strncmp(pPath, "/api/file/", 10) == 0 && strcmp(pMethod, "GET") == 0)
	{
		const char* filename = pPath + 10;
		json << "{\"filename\":\"" << filename << "\",\"data\":\"base64data\",\"size\":1024}";
	}
	else if (strcmp(pPath, "/api/upload") == 0 && strcmp(pMethod, "POST") == 0)
	{
		json << "{\"success\":true,\"message\":\"File uploaded successfully\"}";
	}
	else if (strcmp(pPath, "/api/peers") == 0 && strcmp(pMethod, "GET") == 0)
	{
		json << "{\"peers\":[{\"id\":\"peer1\",\"ip\":\"192.168.1.100\",\"port\":8847}],\"count\":1}";
	}
	else
	{
		json << "{\"error\":\"Unknown API endpoint\",\"path\":\"" << pPath << "\"}";
	}

	return json.str();
}

/**
* @brief Get HTTP port from registry configuration
*/
DWORD CWindowsService::GetHttpPortFromRegistry()
{
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

/**
* @brief Install service in SCM
*/
BOOL CWindowsService::InstallService()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		_tprintf(_T("Cannot install service (%d)\n"), GetLastError());
		return FALSE;
	}

	// Get a handle to the SCM database
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		_tprintf(_T("OpenSCManager failed (%d)\n"), GetLastError());
		return FALSE;
	}

	// Create the service
	schService = CreateService(
		schSCManager,              // SCM database 
		SERVICE_NAME,              // name of service 
		SERVICE_DISPLAY_NAME,      // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL)
	{
		_tprintf(_T("CreateService failed (%d)\n"), GetLastError());
		CloseServiceHandle(schSCManager);
		return FALSE;
	}
	else
	{
		_tprintf(_T("Service installed successfully\n"));
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

/**
* @brief Uninstall service from SCM
*/
BOOL CWindowsService::UninstallService()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		_tprintf(_T("OpenSCManager failed (%d)\n"), GetLastError());
		return FALSE;
	}

	// Get a handle to the service
	schService = OpenService(
		schSCManager,       // SCM database 
		SERVICE_NAME,       // name of service 
		DELETE);            // need delete access 

	if (schService == NULL)
	{
		_tprintf(_T("OpenService failed (%d)\n"), GetLastError());
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Delete the service
	if (!DeleteService(schService))
	{
		_tprintf(_T("DeleteService failed (%d)\n"), GetLastError());
	}
	else
	{
		_tprintf(_T("Service deleted successfully\n"));
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

/**
* @brief Update and report service status to SCM
*/
void CWindowsService::SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	m_ServiceStatus.dwCurrentState = dwCurrentState;
	m_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	m_ServiceStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		m_ServiceStatus.dwControlsAccepted = 0;
	else
		m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		m_ServiceStatus.dwCheckPoint = 0;
	else
		m_ServiceStatus.dwCheckPoint = dwCheckPoint++;

	::SetServiceStatus(m_StatusHandle, &m_ServiceStatus);
}

/**
* @brief Write message to Windows Event Log
*/
void CWindowsService::WriteToEventLog(LPCTSTR pszMessage, WORD wType)
{
	HANDLE hEventSource = RegisterEventSource(NULL, SERVICE_NAME);
	if (hEventSource != NULL)
	{
		ReportEvent(hEventSource, wType, 0, 0, NULL, 1, 0, &pszMessage, NULL);
		DeregisterEventSource(hEventSource);
	}
}
