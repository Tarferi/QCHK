#include "Storm.h"
#ifndef _DEBUG
#include "SancData.h"
#endif

bool decompressLib(char** data, unsigned int* size) {
	bool error = false;
	char* newData;
	unsigned int newSize;
	decompress((char*) _acStormLib, _acStormLibLen, &newData, &newSize, &error);
	*data = newData;
	*size = newSize;
	return !error;
}

Storm::Storm(bool* error) {
	this->lib = nullptr;
	unsigned int libSize;
	if (!decompressLib(&this->decompressedLib, &libSize)) {
		*error = true;
		return;
	}

	this->lib = MemoryLoadLibrary(this->decompressedLib, libSize);
	if (!this->lib) {
		throw 1;
	}
	SFileOpenArchive = (SFileOpenArchiveF)MemoryGetProcAddress(this->lib, "SFileOpenArchive");
	SFileFindNextFile = (SFileFindNextFileF)MemoryGetProcAddress(this->lib, "SFileFindNextFile");
	SFileFindFirstFile = (SFileFindFirstFileF)MemoryGetProcAddress(this->lib, "SFileFindFirstFile");
	SFileFindClose = (SFileFindCloseF)MemoryGetProcAddress(this->lib, "SFileFindClose");
	SFileExtractFile = (SFileExtractFileF)MemoryGetProcAddress(this->lib, "SFileExtractFile");
	SFileReadFile = (SFileReadFileF)MemoryGetProcAddress(this->lib, "SFileReadFile");
	SFileOpenFileEx = (SFileOpenFileExF)MemoryGetProcAddress(this->lib, "SFileOpenFileEx");
	SFileCloseFile = (SFileCloseFileF)MemoryGetProcAddress(this->lib, "SFileCloseFile");
	SFileCreateFile = (SFileCreateFileF)MemoryGetProcAddress(this->lib, "SFileCreateFile");
	SFileWriteFile = (SFileWriteFileF)MemoryGetProcAddress(this->lib, "SFileWriteFile");
	SFileFinishFile = (SFileFinishFileF)MemoryGetProcAddress(this->lib, "SFileFinishFile");
	SFileCreateArchive = (SFileCreateArchiveF)MemoryGetProcAddress(this->lib, "SFileCreateArchive");
	SFileCloseArchive = (SFileCloseArchiveF)MemoryGetProcAddress(this->lib, "SFileCloseArchive");
}

Storm::~Storm() {
	if (this->decompressedLib != nullptr) {
		free(this->decompressedLib);
		this->decompressedLib = nullptr;
	}
	if (this->lib != nullptr) {
		MemoryFreeLibrary(this->lib);
		this->lib = nullptr;
	}
}

MapFile * Storm::readSCX(char * filePath, bool* error)
{

	HANDLE mapFile;
	void* scx = SFileOpenArchive(filePath, 0, 0x00000100, &mapFile);
	if (!scx) {
		LOG("STORM", "Failed to open file %s", filePath)
		throw 1;
	}
	LOG("STORM", "Opened file %s", filePath);

	SFILE_FIND_DATA data;

	HANDLE searchHandle = SFileFindFirstFile(mapFile, "*", &data, (const char*)0);

	Array<char*>* fileNames = new Array<char*>();
	Array<unsigned int>* fileSizes = new Array<unsigned int>();
	Array<char*>* filesContents = new Array<char*>();

	bool hasScenarioChk = false;

	do {
		
		fileSizes->append(data.dwFileSize);
		GET_CLONED_STRING(fileName, data.cFileName, { fileNames->freeItems(); filesContents->freeItems(); SFileFindClose(searchHandle); SFileCloseArchive(mapFile); delete fileNames; delete fileSizes; delete filesContents; return nullptr; });
		fileNames->append(fileName);
		HANDLE fileH;
		MALLOC_N(fileContents, char, data.dwFileSize, { fileNames->freeItems(); filesContents->freeItems(); SFileFindClose(searchHandle); SFileCloseArchive(mapFile); delete fileNames; delete fileSizes; delete filesContents; return nullptr; });
		SFileOpenFileEx(mapFile, data.cFileName, 0, &fileH);
		DWORD read;
		SFileReadFile(fileH, fileContents, data.dwFileSize, &read, 0);
		LOG("STORM", "Read file %s from %s", data.cFileName, filePath);
		if (read != data.dwFileSize) {
			LOG("STORM", "Read only %d of %d total\n", read, data.dwFileSize);
		}
		SFileCloseFile(fileH);
		filesContents->append(fileContents);

	} while (SFileFindNextFile(searchHandle, &data));

	SFileFindClose(searchHandle);
	SFileCloseArchive(mapFile);
	LOG("STORM", "Closed file %s", filePath);
	MapFile* mf = new MapFile(filesContents, fileSizes, fileNames, error);
	return mf;
}

#define HEXDIGIT(d) (d - 'A')


void process(char* fileName, unsigned int fileSize, char* data, unsigned int *position, Array<char*>* fileNames, Array<unsigned int>* fileSizes, Array<char*>* filesContents, bool* error) {
	unsigned int begin = *position;
	fileSizes->append(fileSize);
	GET_CLONED_STRING(newFileName, fileName, { *error = true; return; });
	fileNames->append(newFileName);
	MALLOC_N(newFileContents, char, fileSize, { *error = true; return; });
	for (unsigned int i = 0; i < fileSize; i++) {
		char byte = data[begin + i];
		byte = byte ^ (i % 256);
		newFileContents[i] = byte;
	}
	filesContents->append(newFileContents);
	*position = begin + fileSize;
}

MapFile * Storm::readSanc(bool* error) {
	char* decompressedSancData;
	unsigned int decompressedSancDataLength;
	decompress((char*) _acdata, 3279744UL + 1, &decompressedSancData, &decompressedSancDataLength, error);
	if (*error) {
		return nullptr;
	}


	Array<char*>* fileNames = new Array<char*>();
	Array<unsigned int>* fileSizes = new Array<unsigned int>();
	Array<char*>* filesContents = new Array<char*>();
	unsigned int pos = 0;
	process((char*)_acfile_0_name, _acfile_0_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_1_name, _acfile_1_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_2_name, _acfile_2_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_3_name, _acfile_3_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_4_name, _acfile_4_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_5_name, _acfile_5_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_6_name, _acfile_6_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);
	process((char*)_acfile_7_name, _acfile_7_length, (char*)decompressedSancData, &pos, fileNames, fileSizes, filesContents, error);

	free(decompressedSancData);

	MapFile* mf = new MapFile(filesContents, fileSizes, fileNames, error);
	return mf;
}

bool Storm::writeSCX(char* file, CHK* chk, SoundCollection* sounds) {
	int files = sounds->files.getSize() + 1;
	if (files < 4) {
		files = 4;
	}
	LOG("STORM", "Export files count: %d", files)
	HANDLE fl;

	remove(file);
	if (!SFileCreateArchive((wchar_t*)file, 0, files, &fl)) {
		LOG("STORM", "Failed creating archive \"%s\"", file);
		return false;
	}
	
	// Write CHK first
	char* chkFile = "staredit\\scenario.chk";
	char* contents;
	unsigned int fileSize;
	WriteBuffer* wb = new WriteBuffer();
	if (!chk->write(wb)) {
		SFileCloseArchive(fl);
		delete wb;
		return false;
	}
	wb->getWrittenData((unsigned char**) (&contents), &fileSize);
	HANDLE fh;
	if (!SFileCreateFile(fl, chkFile, 0, fileSize, 0, 0, &fh)) {
		SFileCloseArchive(fl);
		delete wb;
		return false;
	}
	if (!SFileWriteFile(fh, contents, fileSize, 0)) {
		SFileFinishFile(fh);
		SFileCloseArchive(fl);
		delete wb;
		return false;
	}
	if (!SFileFinishFile(fh)) {
		SFileCloseArchive(fl);
		delete wb;
		return false;
	}
	delete wb;


	// Write every other files
	for (unsigned int fileIndex = 0; fileIndex < sounds->files.getSize(); fileIndex++) {
		SoundFile* file = sounds->files[fileIndex];
		if (file->v2Index != 0) {
			LOG("STORM", "Found file \"%s\" with index, exporting", file->fileName)
			Section_STR_* STR = (Section_STR_*)chk->getSection("STR ");
			if (STR == nullptr) { // Should never happen tho
				LOG("STORM", "STR section not found while saving \"%s\"", file->fileName);
				SFileCloseArchive(fl);
				return false;
			}

			char* fileName = STR->getRawString(file->v2Index);
			char* contents = file->contents;
			unsigned int fileSize = file->contentsSize;

			HANDLE fh;

			if (!SFileCreateFile(fl, fileName, 0, fileSize, 0, 0, &fh)) {
				LOG("STORM", "Failed creating file \"%s\" in archive \"%s\"", fileName, file->fileName);
				SFileCloseArchive(fl);
				return false;
			}
			if (!SFileWriteFile(fh, contents, fileSize, 0)) {
				LOG("STORM", "Failed writing file \"%s\" (%d bytes) in archive \"%s\"", fileName, fileSize, file->fileName);
				SFileFinishFile(fh);
				SFileCloseArchive(fl);
				return false;
			}
			SFileFinishFile(fh);
		}
	}
	SFileCloseArchive(fl);
	return true;
}
