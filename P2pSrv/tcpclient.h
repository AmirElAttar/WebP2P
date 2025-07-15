#pragma once

#include <winsock2.h>
#include <string>

#include "tcpdef.h"



class TCPFileClient {
private:
	SOCKET m_socket;
	std::string m_serverIP;
	int m_serverPort;
	bool m_connected;

	DWORD CalculateSimpleCRC32(const char* data, DWORD size);

public:
	TCPFileClient(const std::string& serverIP, int serverPort);
	~TCPFileClient();
	void WriteToEventLog(const char* pszMessage);
	bool Initialize();
	bool Connect();
	void Disconnect();
	bool ConnectWithPortDiscovery();
	bool DownloadFile(const std::string& filename, const std::string& outputPath);
	bool DownloadFileFromServer(const std::string& serverIP, const std::string& filename, const std::string& outputPath);
};

// Helper functions


std::string trim(const std::string& str);

std::string generateOutputPath(const std::string& filename);

