#pragma once
#include "common.h"
#include "CHK.h"

class Storm;

class MapFile {

public:
	MapFile(Array<MapFileStr*>* files, bool* error, bool isSanc);
	~MapFile();

	CHK* getCHK();
	void writeToFile(Storm* storm, char* name, bool* error);

	Array<MapFileStr*>* files = nullptr;

private:
	CHK* chk;
	SoundCollection* sounds = nullptr;
	bool isSanc;

};

