#pragma once

#include "common.h"
#include "Section.h"
#include "ReadBuffer.h"
#include "SoundCollection.h"

class MapFile;

class CHK
{
public:
	CHK(SoundCollection* sounds, FILE* file);
	CHK(SoundCollection* sounds, const char* fileName);
	CHK(SoundCollection* sounds, char* data, unsigned int size);
	~CHK();
	bool write(WriteBuffer* buffer);
	bool write(const char* file);
	Section* getSection(const char* name);
	void removeSection(Section* section);
	void setSection(Section* section, bool* errpr);
	Array<unsigned short>* getUsedWavs(bool* error) {
		return getUsedWavs(nullptr, error);
	}
	Array<unsigned short>* getUsedWavs(Array<Action*>* actionSetter, bool* error);
	Array<Section*> sections;
	SoundCollection* getSounds() {
		return this->sounds;
	}
	bool isValid() {
		return this->valid;
	}
private:
	
	ReadBuffer* buffer;
	bool closeFileAfter = false;
	FILE* file;
	SoundCollection* sounds = nullptr;
	bool valid;
	bool parse();
};

