#pragma once

#include "Mem.h"
#include "common.h"
#include "CHK.h"
#include "StormLib.h"
#include "MapFile.h"
#include "atlstr.h"
#include "SoundCollection.h"
#include "SancData.h"

// #define STORM_DUMP_FILES

typedef void* (WINAPI *SFileOpenArchiveF)(const char* szMpqName, DWORD, DWORD, HANDLE);

typedef void(*FNPTR)(char*);

typedef struct SFILE_FIND_DATA {
	char   cFileName[MAX_PATH];              // Name of the found file
	char* szPlainName;                      // Plain name of the found file
	DWORD  dwHashIndex;                      // Hash table index for the file
	DWORD  dwBlockIndex;                     // Block table index for the file
	DWORD  dwFileSize;                       // Uncompressed size of the file, in bytes
	DWORD  dwFileFlags;                      // MPQ file flags
	DWORD  dwCompSize;                       // Compressed file size
	DWORD  dwFileTimeLo;                     // Low 32-bits of the file time (0 if not present)
	DWORD  dwFileTimeHi;                     // High 32-bits of the file time (0 if not present)
	LCID   lcLocale;                         // Locale version
} SFILE_FIND_DATA;

typedef bool (WINAPI *SFileFindNextFileF) (HANDLE hFind, SFILE_FIND_DATA*);

typedef HANDLE(WINAPI *SFileFindFirstFileF) (HANDLE hFind, const char*, SFILE_FIND_DATA*, const char*);

typedef bool (WINAPI *SFileFindCloseF) (HANDLE hFind);

typedef bool (WINAPI *SFileExtractFileF) (HANDLE hMpq, const char * szToExtract, const char * szExtracted, DWORD dwSearchScope);

typedef bool (WINAPI *SFileReadFileF) (HANDLE hFile, VOID * lpBuffer, DWORD dwToRead, DWORD * pdwRead, LPOVERLAPPED lpOverlapped);

typedef bool (WINAPI *SFileOpenFileExF) (HANDLE hMpq, const char * szFileName, DWORD dwSearchScope, HANDLE * phFile);

typedef bool (WINAPI *SFileCloseFileF) (HANDLE hFile);

typedef bool (WINAPI *SFileCreateFileF) (HANDLE hMpq, const char * szArchivedName, ULONGLONG FileTime, DWORD dwFileSize, LCID lcLocale, DWORD dwFlags, HANDLE * phFile);

typedef bool (WINAPI *SFileWriteFileF) (HANDLE hFile, const void * pvData, DWORD dwSize, DWORD dwCompression);

typedef bool (WINAPI *SFileFinishFileF) (HANDLE hFile);

typedef bool (WINAPI *SFileCreateArchiveF) (const TCHAR * szMpqName, DWORD dwCreateFlags, DWORD dwMaxFileCount, HANDLE * phMPQ);

typedef bool (WINAPI *SFileCloseArchiveF) (HANDLE hMpq);

class Storm
{
public:
	Storm(bool* error);

	~Storm();

	MapFile* readSCX(char* filePath, bool* error);

	MapFile* readSanc(bool* error);

	bool writeSCX(char* file, CHK* chk, SoundCollection* sounds);

private:
	char* decompressedLib = nullptr;

	HMEMORYMODULE lib = nullptr;
	SFileOpenArchiveF SFileOpenArchive;
	SFileFindNextFileF SFileFindNextFile;
	SFileFindFirstFileF SFileFindFirstFile;
	SFileFindCloseF SFileFindClose;
	SFileExtractFileF SFileExtractFile;

	SFileReadFileF SFileReadFile;
	SFileOpenFileExF SFileOpenFileEx;
	SFileCloseFileF SFileCloseFile;

	SFileCreateFileF SFileCreateFile;
	SFileWriteFileF SFileWriteFile;
	SFileFinishFileF SFileFinishFile;
	SFileCreateArchiveF SFileCreateArchive;
	SFileCloseArchiveF SFileCloseArchive;
};

