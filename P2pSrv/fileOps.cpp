#include "fileOps.h"

#include <sstream>
#include <iomanip>

#pragma comment(lib, "bcrypt.lib")

void localFileHandler::calcHash()
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapping = NULL;
	LPVOID pFileData = NULL;
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	NTSTATUS status;
	DWORD hashObjectSize = 0, cbData = 0;
	std::vector<BYTE> hashObject;
	std::vector<BYTE> hash(32); // SHA-256 = 32 bytes

	// Open the file
	hFile = CreateFileA(
		fileName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open file.\n";
		return;
	}

	// Create a file mapping
	hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hMapping) {
		std::cerr << "Failed to create file mapping.\n";
		CloseHandle(hFile);
		return;
	}

	// Map the file into memory
	pFileData = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!pFileData) {
		std::cerr << "Failed to map view of file.\n";
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return;
	}

	// Open SHA-256 algorithm provider
	status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
	if (!BCRYPT_SUCCESS(status)) {
		std::cerr << "Failed to open algorithm provider.\n";
		goto Cleanup;
	}

	// Get hash object size
	status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &cbData, 0);
	if (!BCRYPT_SUCCESS(status)) {
		std::cerr << "Failed to get hash object size.\n";
		goto Cleanup;
	}

	hashObject.resize(hashObjectSize);

	// Create hash object
	status = BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, NULL, 0, 0);
	if (!BCRYPT_SUCCESS(status)) {
		std::cerr << "Failed to create hash object.\n";
		goto Cleanup;
	}

	// Get file size
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize)) {
		std::cerr << "Failed to get file size.\n";
		goto Cleanup;
	}

	// Hash the file data
	status = BCryptHashData(hHash, static_cast<PUCHAR>(pFileData), static_cast<ULONG>(fileSize.QuadPart), 0);
	if (!BCRYPT_SUCCESS(status)) {
		std::cerr << "Failed to hash file data.\n";
		goto Cleanup;
	}

	// Finalize the hash
	status = BCryptFinishHash(hHash, hash.data(), static_cast<ULONG>(hash.size()), 0);
	if (!BCRYPT_SUCCESS(status)) {
		std::cerr << "Failed to finish hash.\n";
		goto Cleanup;
	}

	// Convert hash to hex string
	char hexBuffer[65] = {};
	for (size_t i = 0; i < hash.size(); ++i) {
		sprintf_s(hexBuffer + i * 2, 3, "%02x", hash[i]);
	}
	sha256Hash = std::string(hexBuffer);

Cleanup:
	if (pFileData) UnmapViewOfFile(pFileData);
	if (hMapping) CloseHandle(hMapping);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	if (hHash) BCryptDestroyHash(hHash);
	if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
}

std::string FileTimeToString(const FILETIME& ft) {
	SYSTEMTIME utc, local;

	// Convert FILETIME to SYSTEMTIME (UTC)
	if (!FileTimeToSystemTime(&ft, &utc)) {
		return "Invalid time";
	}

	// Convert UTC to local time
	if (!SystemTimeToTzSpecificLocalTime(NULL, &utc, &local)) {
		local = utc; // Fallback to UTC if conversion fails
	}

	// Format as string
	std::ostringstream oss;
	oss << std::setfill('0')
		<< std::setw(4) << local.wYear << "-"
		<< std::setw(2) << local.wMonth << "-"
		<< std::setw(2) << local.wDay << " "
		<< std::setw(2) << local.wHour << ":"
		<< std::setw(2) << local.wMinute << ":"
		<< std::setw(2) << local.wSecond;

	return oss.str();
}

std::string localFileHandler::getCreationDate()
{
	return FileTimeToString(ftCreationTime);
}
std::string localFileHandler::getLastWriteDate()
{
	return FileTimeToString(ftLastWriteTime);
}

std::string localFileHandler::getshortName()
{
	size_t pos = fileName.find_last_of("\\/");
	if (pos != std::string::npos) {
		return fileName.substr(pos + 1);
	}
	return fileName;  // No path separator found, return as-is
}


void localFileHandler::setfileSize(DWORD lo, DWORD hi) 
{
	LARGE_INTEGER li;
	li.LowPart = lo;
	li.HighPart = hi;
	fileSize=li.QuadPart;
}