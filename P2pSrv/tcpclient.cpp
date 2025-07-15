#include "tcpclient.h"

#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <strsafe.h>


#pragma comment(lib, "ws2_32.lib")

/**
* @brief Write message to Windows Event Log
*/
void TCPFileClient::WriteToEventLog(const char* pszMessage)
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



// Trim whitespace from string
std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (first == std::string::npos) {
		return "";
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}


// Generate output path
std::string generateOutputPath(const std::string& filename) {
	return "C:\\Downloads\\" + filename;
}



// Constructor
TCPFileClient::TCPFileClient(const std::string& serverIP, int serverPort)
	: m_serverIP(serverIP), m_serverPort(serverPort), m_connected(false) {
	m_socket = INVALID_SOCKET;
}

// Destructor
TCPFileClient::~TCPFileClient() {
	Disconnect();
}

// Initialize Windows Sockets
bool TCPFileClient::Initialize() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		WriteToEventLog("WSAStartup failed");
		return false;
	}
	return true;
}

// Connect to server
bool TCPFileClient::Connect() {
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		WriteToEventLog("Socket creation failed");
		return false;
	}

	DWORD timeout = 30000;
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(m_serverPort);

	if (inet_pton(AF_INET, m_serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
		WriteToEventLog("Invalid server IP address");
		closesocket(m_socket);
		return false;
	}

	if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(m_socket);
		return false;
	}

	m_connected = true;
	return true;
}

// Disconnect from server
void TCPFileClient::Disconnect() {
	if (m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	m_connected = false;
}

// Connect with port discovery
bool TCPFileClient::ConnectWithPortDiscovery() {
	std::vector<int> ports = { 8080, 9000, 8888, 9001, 9002 };
	WriteToEventLog("Discovering server port...");

	for (int port : ports) {
		m_serverPort = port;
		std::string msg = "Trying to connect to port " + std::to_string(port) + "...";
		WriteToEventLog(msg.c_str());

		if (Connect()) {
			WriteToEventLog("Successfully connected to server");
			return true;
		}
		else {
			WriteToEventLog("Failed to connect, trying next...");
		}
	}

	WriteToEventLog("Could not connect to server on any available port");
	return false;
}

// Download file from connected server
bool TCPFileClient::DownloadFile(const std::string& filename, const std::string& outputPath) {
	if (!m_connected) {
		WriteToEventLog("Not connected to server");
		return false;
	}

	std::ofstream outputFile(outputPath, std::ios::binary | std::ios::trunc);
	if (!outputFile.is_open()) {
		WriteToEventLog("Cannot create output file");
		return false;
	}

	DWORD chunkIndex = 0;
	DWORD totalChunks = 0;
	bool firstChunk = true;

	std::string msg = "Downloading " + filename + "...";
	WriteToEventLog(msg.c_str());

	while (true) {
		ChunkRequest request;
		request.msgType = MSG_CHUNK_REQUEST;
		strncpy_s(request.filename, filename.c_str(), MAX_FILENAME - 1);
		request.filename[MAX_FILENAME - 1] = '\0';
		request.chunkIndex = chunkIndex;
		request.reserved = 0;

		if (send(m_socket, (char*)&request, sizeof(request), 0) == SOCKET_ERROR) {
			WriteToEventLog("Failed to send request");
			return false;
		}

		ChunkResponse response;
		int bytesReceived = recv(m_socket, (char*)&response, sizeof(response), MSG_WAITALL);
		if (bytesReceived != sizeof(response)) {
			WriteToEventLog("Failed to receive response header");
			return false;
		}

		if (response.msgType == MSG_FILE_NOT_FOUND) {
			WriteToEventLog("File not found on server");
			return false;
		}

		if (response.msgType == MSG_ERROR) {
			WriteToEventLog("Server error occurred");
			return false;
		}

		if (response.msgType != MSG_CHUNK_RESPONSE) {
			WriteToEventLog("Invalid response type");
			return false;
		}

		if (firstChunk) {
			totalChunks = response.totalChunks;
			firstChunk = false;
			msg = "File has " + std::to_string(totalChunks) + " chunks";
			WriteToEventLog(msg.c_str());
		}

		std::vector<char> chunkData(response.chunkSize);
		bytesReceived = recv(m_socket, chunkData.data(), response.chunkSize, MSG_WAITALL);
		if (bytesReceived != (int)response.chunkSize) {
			WriteToEventLog("Failed to receive chunk data");
			return false;
		}

		DWORD calculatedCRC = CalculateSimpleCRC32(chunkData.data(), response.chunkSize);
		if (calculatedCRC != response.crc32) {
			WriteToEventLog("Chunk CRC mismatch - data corruption detected");
			return false;
		}

		outputFile.write(chunkData.data(), response.chunkSize);

		int progressPercent = (int)(((chunkIndex + 1) * 100) / totalChunks);
		msg = "Progress: " + std::to_string(chunkIndex + 1) + "/" + std::to_string(totalChunks) +
			" chunks (" + std::to_string(progressPercent) + "%)";
		WriteToEventLog(msg.c_str());

		chunkIndex++;
		if (chunkIndex >= totalChunks) {
			break;
		}
	}

	outputFile.close();
	WriteToEventLog("Download completed successfully");
	return true;
}

// Calculate simple CRC32 checksum
DWORD TCPFileClient::CalculateSimpleCRC32(const char* data, DWORD size) {
	DWORD checksum = 0;
	for (DWORD i = 0; i < size; i++) {
		checksum += (unsigned char)data[i];
	}
	return checksum;
}

// Download file from specific server
bool TCPFileClient::DownloadFileFromServer(const std::string& serverIP, const std::string& filename, const std::string& outputPath) {
	std::string originalServerIP = m_serverIP;
	m_serverIP = serverIP;

	std::string msg = "Starting download from " + serverIP + ": " + filename;
	WriteToEventLog(msg.c_str());

	if (!ConnectWithPortDiscovery()) {
		WriteToEventLog("Failed to connect to server");
		m_serverIP = originalServerIP;
		return false;
	}

	bool result = DownloadFile(filename, outputPath);
	Disconnect();
	m_serverIP = originalServerIP;

	if (result) {
		WriteToEventLog("Download successful");
	}
	else {
		WriteToEventLog("Download failed");
	}

	return result;
}

