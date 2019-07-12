#pragma once
#include "common.h"
#include "CHK.h"

class Storm;

class MapFile
{
public:
	MapFile(Array<char*>* data, Array<unsigned int>* dataLengths, Array<char*>* fileNames, bool* error);
	~MapFile();

	CHK* getCHK();
	void writeToFile(Storm* storm, char* name, bool* error);

	Array<char*>* contents;
	Array<unsigned int>* dataLengths;
	Array<char*>* fileNames;

	void dump();

private:
	CHK* chk;
	SoundCollection* sounds = nullptr;

};

