#ifndef WINDOWS_SERVICE_H
#define WINDOWS_SERVICE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <http.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Service configuration
#define SERVICE_NAME        _T("P2pWindowsService")
#define SERVICE_DISPLAY_NAME _T("P2p file sharing Service")
#define DEFAULT_HTTP_PORT   8847
#define HTTP_URL_PREFIX     L"http://+:%d/"
#define MAX_REQUEST_SIZE    4096

/**
* @brief Minimal Windows Service with HTTP API server
*/
class CWindowsService
{
public:
	/**
	* @brief Default constructor
	*/
	CWindowsService();

	/**
	* @brief Virtual destructor
	*/
	virtual ~CWindowsService();

	/**
	* @brief Main service entry point called by SCM
	*/
	static void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);

	/**
	* @brief Service control handler for SCM commands
	*/
	static void WINAPI ServiceCtrlHandler(DWORD ctrl);

	/**
	* @brief Worker thread that runs the HTTP API server
	*/
	static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

	/**
	* @brief Start service programmatically (placeholder)
	*/
	void Start();

	/**
	* @brief Stop service programmatically (placeholder)
	*/
	void Stop();

	/**
	* @brief Install service in SCM
	*/
	static BOOL InstallService();

	/**
	* @brief Uninstall service from SCM
	*/
	static BOOL UninstallService();

private:
	// Service status members
	static SERVICE_STATUS        m_ServiceStatus;
	static SERVICE_STATUS_HANDLE m_StatusHandle;
	static HANDLE                m_ServiceStopEvent;

	// HTTP Server members
	static HANDLE                m_hHttpQueue;
	static HTTP_SERVER_SESSION_ID m_SessionId;
	static HTTP_URL_GROUP_ID     m_UrlGroupId;
	static DWORD                 m_HttpPort;

	/**
	* @brief Initialize HTTP server components
	*/
	static DWORD InitializeHttpServer();

	/**
	* @brief Clean up HTTP server resources
	*/
	static void CleanupHttpServer();

	/**
	* @brief Process incoming HTTP requests
	*/
	static DWORD ProcessHttpRequest(PHTTP_REQUEST pRequest, HTTP_REQUEST_ID RequestId);

	/**
	* @brief Send JSON response with CORS headers
	*/
	static DWORD SendJsonResponse(HTTP_REQUEST_ID RequestId, USHORT StatusCode, const char* pJsonContent);

	/**
	* @brief Handle API endpoint requests
	*/
	static std::string HandleApiRequest(const char* pPath, const char* pMethod);

	/**
	* @brief Get HTTP port from registry configuration
	*/
	static DWORD GetHttpPortFromRegistry();

	/**
	* @brief Update and report service status to SCM
	*/
	static void SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

	/**
	* @brief Write message to Windows Event Log
	*/
	static void WriteToEventLog(LPCTSTR pszMessage, WORD wType);
};

#endif // WINDOWS_SERVICE_H
