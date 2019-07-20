#pragma once
#include "common.h"
#include "Section.h"

#define IS_OGG(fileName) (!_stricmp((char*) (fileName+(strlen(fileName)-3)), "ogg"))

struct SoundFile {
	char* fileName;
	char* contents;
	unsigned int contentsSize;
	bool isOgg;
	unsigned short v2Index;

};

class SoundCollection
{
public:
	SoundCollection(Array<MapFileStr*>* files, bool* error);
	~SoundCollection();

	void deleteSound(char* fileName);

	unsigned short addOrRewriteSound(Section_STR_* STR, char* fileName, char* contents, unsigned int contentsLength, bool isOgg, bool* error);
	
	bool getSound(char* fileName, SoundFile** filePtr);

	Array<SoundFile*> files;

private:

	bool getSound(char* fileName, SoundFile** filePtr, unsigned int* fileIndex);
};

