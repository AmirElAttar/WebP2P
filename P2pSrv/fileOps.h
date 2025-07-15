#ifndef __FILE_OPS__
#define __FILE_OPS__
#include <windows.h>  //for file API's
#include <bcrypt.h>  //for sha-256 encryption
#include <fileapi.h> //for file attribute struct
#include <string>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace std;

class localFileHandler
{
private:

	std::string fileName;	// Full file name (with path)
	std::string sha256Hash;	// SHA-256 hash as file fingerprint
	FILETIME ftCreationTime;
	FILETIME ftLastWriteTime;
	ULONG64 fileSize;
	

public:
	// Default constructor
	localFileHandler()
		: fileName(""), sha256Hash("")
	{
		
	}

	// Parameterized constructor
	localFileHandler(const std::string& name)
		: fileName(name)
	{
		calcHash();
	}

	// Destructor
	~localFileHandler() = default;

	// Getters
	std::string getFileName()  { return fileName; }
	std::string getshortName();
	std::string getHash()  { return sha256Hash; }
	std::string getCreationDate();
	std::string getLastWriteDate();
	std::string getFileSize() { return std::to_string(fileSize); }
	void calcHash();

	// Setters
	void setCreationDate(FILETIME t){ ftCreationTime = t; }
	void setWriteTime(FILETIME t){ ftLastWriteTime = t; }
	void setfileSize(DWORD lo, DWORD hi);

	/*void setFileName(const std::string& name) { fileName = name; }
	void setFileIndex(uint64_t index) { fileIndex = index; }
	void setFileInfo(const WIN32_FILE_ATTRIBUTE_DATA& info) { fileInfo = info; }
	void calcHash();
	// Utility: Reset file index
	void resetIndex() { fileIndex = 0; }*/
};





#endif  //__FILE_OPS__