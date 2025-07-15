#pragma once
#include <windows.h>
#define CHUNK_SIZE 65536
#define MAX_FILENAME 256
// Protocol message types
enum MessageType {
	MSG_CHUNK_REQUEST = 1,
	MSG_CHUNK_RESPONSE = 2,
	MSG_FILE_NOT_FOUND = 3,
	MSG_ERROR = 4
};

struct ChunkRequest {
	MessageType msgType;
	char filename[MAX_FILENAME];
	DWORD chunkIndex;
	DWORD reserved;
};

struct ChunkResponse {
	MessageType msgType;
	DWORD chunkIndex;
	DWORD chunkSize;
	DWORD totalChunks;
	DWORD crc32;
};