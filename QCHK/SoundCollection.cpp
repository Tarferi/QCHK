#include "SoundCollection.h"




SoundCollection::SoundCollection(Array<MapFileStr*>* files, bool* error)
{
	for (unsigned int fileIndex = 0; fileIndex < files->getSize(); fileIndex++) {

		GET_CLONED_DATA(newContents, char, files->get(fileIndex)->contents, files->get(fileIndex)->contentsLength, { *error = true; return; });
		GET_CLONED_STRING(newFilename, files->get(fileIndex)->fileName, { free(newContents); *error = true; return; });
		MALLOC_N(file, SoundFile, 1, { free(newContents); free(newFilename); *error = true; return; });
		file->contents = newContents;
		file->contentsSize = files->get(fileIndex)->contentsLength;
		file->fileName = newFilename;
		file->v2Index = 0;
		file->isOgg = IS_OGG(file->fileName);
		if (!this->files.append(file)) {
			free(newFilename);
			free(newContents);
			free(file);
			*error = true;
		}
	}
}

SoundCollection::~SoundCollection()
{
	for (unsigned int fileIndex = 0; fileIndex < this->files.getSize(); fileIndex++) {
		SoundFile* file = files[fileIndex];
		char* currentContents = file->contents;
		char* currentName = file->fileName;
		free(currentContents);
		free(currentName);
		free(file);
	}
}

void SoundCollection::deleteSound(char * fileName)
{
	unsigned int index;
	SoundFile* file;
	if (getSound(fileName, &file, &index)) {
		this->files.remove(index);
		char* currentContents = file->contents;
		char* currentName = file->fileName;
		LOG("SOUND COLLECTION", "Deleting sound %s", fileName)
			free(currentContents);
		free(currentName);
		free(file);
	}
}

unsigned short SoundCollection::addOrRewriteSound(Section_STR_ * STR, char * fileName, char * contents, unsigned int contentsLength, bool isOgg, bool* error)
{
	unsigned int index;
	SoundFile* originalFile;
	if (getSound(fileName, &originalFile, &index)) { // Rewrite
		SoundFile* file = files[index];
		char* currentContents = file->contents;
		if (currentContents != contents) { // New content
			GET_CLONED_DATA(newContents, char, contents, contentsLength, { *error = true; return 0; });
			free(file->contents);
			file->contents = newContents;
			file->contentsSize = contentsLength;
			file->isOgg = isOgg;
		}
		return file->v2Index;
	}
	else { // Append
		GET_CLONED_DATA(newContents, char, contents, contentsLength, { *error = true; return 0; });
		GET_CLONED_STRING(newFilename, fileName, { free(newContents); *error = true; return 0; });
		MALLOC_N(file, SoundFile, 1, { free(newContents); free(newFilename); *error = true; return 0; });
		file->contents = newContents;
		file->contentsSize = contentsLength;
		file->isOgg = isOgg;
		file->fileName = newFilename;
		char buffer[1024];
		sprintf_s(buffer, "snd_%d.%s", this->files.getSize(), isOgg ? "ogg" : "wav");
		file->v2Index = STR->getNewStringIndex(buffer, error);
		if (*error) {
			free(file->fileName);
			free(file->contents);
			free(file);
		} else if (!this->files.append(file)) {
			free(file->fileName);
			free(file->contents);
			free(file);
			*error = true;
		}
		return file->v2Index;
	}
}

bool SoundCollection::getSound(char * fileName, SoundFile ** filePtr)
{
	unsigned int fileIndex;
	return this->getSound(fileName, filePtr, &fileIndex);
}

bool SoundCollection::getSound(char * fileName, SoundFile ** filePtr, unsigned int * fileIndex)
{
	for (unsigned int soundIndex = 0; soundIndex < files.getSize(); soundIndex++) {
		SoundFile* file = files[soundIndex];
		char* sound = file->fileName;
		if (!strcmp(sound, fileName)) {
			*filePtr = file;
			*fileIndex = soundIndex;
			return true;
		}
	}
	return false;
}