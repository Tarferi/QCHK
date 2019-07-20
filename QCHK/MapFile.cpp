#include "MapFile.h"
#include "Storm.h"


MapFile::MapFile(Array<MapFileStr*>* files, bool* error) {
	this->chk = nullptr;
	this->files = files;
	this->sounds = new SoundCollection(files, error);
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
	if (this->files != nullptr) {
		destroyFileArray(this->files);
		this->files = nullptr;
	}
}

CHK * MapFile::getCHK()
{
	if (this->chk == nullptr) {
		for (unsigned int i = 0; i < this->files->getSize(); i++) {
			if (!strcmp(this->files->get(i)->fileName, "staredit\\scenario.chk")) {
				this->chk = new CHK(this->sounds, (char*) this->files->get(i)->contents, this->files->get(i)->contentsLength);
			}
		}
	}
	return this->chk;
}

bool hasFile(MapFile* mf, char* name) {
	for (unsigned int wavIndex = 0; wavIndex < mf->files->getSize(); wavIndex++) {
		char* wavFile = mf->files->get(wavIndex)->fileName;
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