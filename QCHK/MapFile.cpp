#include "MapFile.h"
#include "Storm.h"


MapFile::MapFile(Array<char*>* data, Array<unsigned int>* dataLengths, Array<char*>* fileNames, bool* error) {
	this->chk = nullptr;
	this->contents = data;
	this->fileNames = fileNames;
	this->dataLengths = dataLengths;
	this->sounds = new SoundCollection(data, dataLengths, fileNames, error);
}

MapFile::~MapFile() {
	if (this->chk != nullptr) {
		delete this->chk;
		this->chk = nullptr;
	}
	if (this->sounds != nullptr) {
		delete this->sounds;
		this->sounds = nullptr;
	}
	if (this->contents != nullptr) {
		for (unsigned int i = 0; i < this->contents->getSize(); i++) {
			char* content = this->contents->get(i);
			if (content != nullptr) {
				free(content);
				this->contents->set(i, nullptr);
			}
		}
		delete this->contents;
	}
	if (this->fileNames != nullptr) {
		for (unsigned int i = 0; i < this->fileNames->getSize(); i++) {
			if (this->fileNames->get(i) != nullptr) {
				free(this->fileNames->get(i));
			}
		}
		delete this->fileNames;
		this->fileNames = nullptr;
	}
	if (this->dataLengths != nullptr) {
		delete this->dataLengths;
	}
}

CHK * MapFile::getCHK()
{
	if (this->chk == nullptr) {
		for (unsigned int i = 0; i < this->fileNames->getSize(); i++) {
			if (!strcmp(this->fileNames->get(i), "staredit\\scenario.chk")) {
				this->chk = new CHK(this->sounds, this->contents->get(i), this->dataLengths->get(i));
			}
		}
	}
	return this->chk;
}

bool hasFile(MapFile* mf, char* name) {
	for (unsigned int wavIndex = 0; wavIndex < mf->fileNames->getSize(); wavIndex++) {
		char* wavFile = mf->fileNames->get(wavIndex);
		if (!strcmp(wavFile, name)) {
			return true;
		}
	}
	return false;
}

void MapFile::writeToFile(Storm* storm, char *name, bool* error) {
	
	// Create new mapfile and write it
	if (!storm->writeSCX(name, this->getCHK(), this->sounds)) {
		*error = true;
	}

}

#ifdef _DEBUG
void MapFile::dump()
{
#ifdef DUMP
	printf("struct SancFile {\r\n\tconst unsigned int length;\r\n\tconst char* name;\r\n\tconst char* data[][];\r\n};\r\n\r\n");
	FILE* f = fopen("data.bin", "wb");
	FILE* f2 = fopen("data.c", "wb");
	for (unsigned int fileIndex = 0; fileIndex < this->contents->getSize(); fileIndex++) {

		char* fileName = this->fileNames->get(fileIndex);
		char* contents = this->contents->get(fileIndex);
		unsigned int contentLength = this->dataLengths->get(fileIndex);
		
		
		fprintf(f2, "static const unsigned int _acfile_%d_length = %d;\r\n", fileIndex, contentLength);
		fprintf(f2, "static const char* _acfile_%d_name = \"%s\";\r\n", fileIndex, fileName);

		
		
		MALLOC_N(fname, char, 1024);
		sprintf(fname, "./file_%d.bin", fileIndex);
		for (unsigned int i = 0; i < contentLength; i++) {
			char byte = contents[i];
			byte = byte ^ (i % 256);
			fwrite(&byte, 1, 1, f);
		}
	
		free(fname);
		continue;

		printf("static const SancFile f_%d = {%d, \"%s\", {\r\n{", fileIndex, contentLength, fileName);
		for (unsigned int i = 0; i < contentLength; i++) {
			unsigned char chr = contents[i];
			printf("0x%02X", chr ^ 0xAA);
			if (i != contentLength - 1) {
				printf(", ");
			}
			if (i % 1024 == 1023) {
				printf("}, \r\n{");
			}
		}
		printf("}\r\n}\r\n};\r\n\r\n\r\n");


	}
	fclose(f2);
	fclose(f);
#endif
}
#endif