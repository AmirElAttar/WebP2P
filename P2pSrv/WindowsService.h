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
#include <vector>
#include <thread>
#include <atomic>
#include "fileOps.h"
// Forward declaration for TCPServer
class TCPFileServer;

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

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

	void StartHttpThrd();

    /**
     * @brief Start TCP server thread
     */
    static void StartTCPServerThrd();

    /**
     * @brief Stop TCP server thread
     */
    void StopTCPServerThrd();


	static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);



private:
	// Service status members
	static SERVICE_STATUS        m_ServiceStatus;
	static SERVICE_STATUS_HANDLE m_StatusHandle;
	static HANDLE                m_ServiceStopEvent;

	// HTTP Server members
	HANDLE hThread;   //httpthread handle
	static HANDLE                m_hHttpQueue;
	static HTTP_SERVER_SESSION_ID m_SessionId;
	static HTTP_URL_GROUP_ID     m_UrlGroupId;
	static DWORD                 m_HttpPort;
	static std::vector<localFileHandler> localFiles;

    // TCP Server member - clean architecture approach
	static TCPFileServer* m_pTCPServer;

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
    static std::string HandleApiRequest(const char* pPath, const char* pMethod, const char* pRequestBody = nullptr);

	/**
	* @brief Get HTTP port from registry configuration
	*/
	static DWORD GetHttpPortFromRegistry();

    // TCP Client integration functions
    static std::string HandleDownloadRequest(const char* pRequestBody);
    static bool DownloadFileFromPeer(const std::string& serverIP, const std::string& filename, const std::string& outputPath);
    static std::string ExtractJsonValue(const std::string& json, const std::string& key);
    static std::string ExtractFirstIP(const std::string& json, const std::string& arrayKey);
    
	/**
	* @brief Write message to Windows Event Log
	*/
	static void WriteToEventLog(char* pszMessage);

	static std::string ShowFolderSelection();
	static void enumerateFiles(std::string);
};

#endif // WINDOWS_SERVICE_H
