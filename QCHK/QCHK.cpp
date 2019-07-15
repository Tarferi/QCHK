// QCHK.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "QCHK.h"

#ifndef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

void processMap(EUDSettings* settings) {
	bool error = false;
	Storm* storm = new Storm(&error);

	MapFile* v2F = storm->readSanc(&error);
	MapFile* v3F = storm->readSCX((char*) settings->inputFilePath, &error);
	if (error || v2F == nullptr || v3F == nullptr) {
		SET_ERROR_LOAD_FILE
			settings->outputFilePath = nullptr;
		if (v2F != nullptr) {
			delete v2F;
		}
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v2 = v2F->getCHK();
	CHK* v3 = v3F->getCHK();

	if (v2 == nullptr || v3 == nullptr) {
		SET_ERROR_LOAD_FILE
		delete v2F;
		delete v3F;
		delete storm;
		return;
	}


	error |= !fix0_disableDefaultAlliances(v2, v3, settings);
	error |= !fix0_relocateStrings(v2, v3, settings);

	if (!settings->useSanctuaryColors) {
		error |= !fix0_fixColors(v2, v3, settings);
	}

	if (settings->addLeaderboard) {
		error |= !fix1_FixDeathsLabel(v2, v3, settings);
	}
	else {
		error |= !fix1_DisableLeaderboard(v2, v3, settings);
	}

	error |= !fix2_FixCenteringViewAtBeginning(v2, v3, settings);
	if (!settings->enableVisor) {
		error |= !fix3_DisableVisor(v2, v3, settings);
	}
	if (!settings->enableBarrier) {
		error |= !fix8_DisableBarrier(v2, v3, settings);
	}
	error |= !fix12_DisableEndGames(v2, v3, settings);
	if (!settings->addTouchRevive) {
		error |= !fix4_DisableTouchRevive(v2, v3, settings);
	}
	if (settings->addTimeLock) {
		error |= !fix5_AddTimeLockTriggers(v2, v3, settings);
	}
	if (settings->recalculateHPAndDamage) {
		error |= !fix13_RecalculateHPAndDamage(v2, v3, settings);
	}
	error |= !fix6_CopyForceNames(v2, v3, settings);
	error |= !fix7_CopyUnitProperties(v2, v3, settings);
	error |= !fix9_RemapLocations(v2, v3, settings);
	error |= !fix10_AddElapsedTimeToAllConditions(v2, v3, settings);
	error |= !fix11_ImportWav(v2, v3, settings);
	error |= !fix14_CopySections(v2, v3, settings);
	error |= !fix15_CopyScenarionNameAndDescription(v2, v3, settings);
	error |= !fix16_CopyTriggersAndBriefing(v2, v3, settings);
	error |= !fix17_CopyUnitSettings(v2, v3, settings);
	error |= !fix18_RelocateSTREUDSection(v2, v3, settings);


	v2F->writeToFile(storm, (char*)settings->outputFilePath, &error);
	
	if (error) {
		SET_ERROR_PROCESS
	}
	
#ifdef TRIG_PRINT
	Section_TRIG* T = (Section_TRIG*)v2->getSection("TRIG");
	Section_STR_* S = (Section_STR_*)v2->getSection("STR ");
	WriteBuffer wb;
	if (T->print(S, &wb)) {
		wb.writeByte(0, &error);
		if (!error) {
			unsigned int length;
			char* data;
			wb.getWrittenData((unsigned char**) &data, &length);
			FILE* f = fopen("triggers.txt", "wb");
			fprintf(f, "%s", data);
			fclose(f);
		}
	}
#endif
	delete v2F;
	delete v3F;
	delete storm;
}

void collectIntoBuffer(Array<char*>* array, WriteBuffer* wb, bool* error) {
	wb->writeInt(array->getSize(), error);
	if (*error) {
		return;
	}
	for (unsigned int i = 0; i < array->getSize(); i++) {
		char* str = array->get(i);
		unsigned int length = strlen(str);
		wb->writeInt(length, error);
		if (*error) {
			return;
		}
		wb->writeArray((unsigned char*) str, length, error);
		if (*error) {
			return;
		}
	}
}

void insertSoundsIntoBuffer(SoundCollection* snd, WriteBuffer* wb, bool* error) {
	wb->writeInt(snd->files.getSize(), error);
	if (*error) {
		return;
	}
	for (unsigned int fileIndex = 0; fileIndex < snd->files.getSize(); fileIndex++) {
		SoundFile* file = snd->files[fileIndex];

		// Write name
		wb->writeInt(strlen(file->fileName), error);
		if (*error) {
			return;
		}
		wb->writeArray((unsigned char*) file->fileName, strlen(file->fileName), error);
		if (*error) {
			return;
		}
		
		// Write length
		wb->writeInt(file->contentsSize, error);
		if (*error) {
			return;
		}
		wb->writeArray((unsigned char*) file->contents, file->contentsSize, error);
		if (*error) {
			return;
		}

		if (ENDS_WIDTH(file->fileName, ".wav")) {
			wb->writeInt(getWavLengthMs(file->contents, error), error);
			if (*error) {
				return;
			}
		}
		else if (ENDS_WIDTH(file->fileName, ".ogg")) {
			wb->writeInt(getOggLengthMs(file->contents, file->contentsSize, error), error);
			if (*error) {
				return;
			}
		}
		else {
			wb->writeInt(0, error);
			if (*error) {
				return;
			}
		}
	}
}

void freeWavs(EUDSettings* settings) {
	if (settings->outputFilePath != nullptr) {
		free(settings->outputFilePath);
		settings->outputFilePath = nullptr;
	}
}

void loadWavs(EUDSettings* settings) {
	SET_NO_ERROR
	bool error = false;
	Storm* storm = new Storm(&error);
	settings->outputFilePath = nullptr;

	MapFile* v2F = storm->readSanc(&error);
	MapFile* v3F = storm->readSCX((char*)settings->inputFilePath, &error);
	if (error || v2F == nullptr || v3F == nullptr) {
		SET_ERROR_LOAD_SECTION
		settings->outputFilePath = nullptr;
		if (v2F != nullptr) {
			delete v2F;
		}
		if (v3F != nullptr) {
			delete v3F;
		}
		delete storm;
		return;
	}

	CHK* v2 = v2F->getCHK();
	CHK* v3 = v3F->getCHK();

	if (v2 == nullptr || v3 == nullptr) {
		SET_ERROR_LOAD_FILE
		delete v2F;
		delete v3F;
		delete storm;
		return;
	}

	SoundCollection* snd = v2->getSounds();
	WriteBuffer* wb = new WriteBuffer();

	insertSoundsIntoBuffer(v2->getSounds(), wb, &error);
	if (!error) {
		insertSoundsIntoBuffer(v3->getSounds(), wb, &error);
		if (!error) {
			unsigned char* data;
			unsigned int length;
			wb->getWrittenData(&data, &length);
			bool error = false;
			GET_CLONED_DATA(result, char, data, length, { error = true; });
			if (!error) {
				settings->outputFilePath = result;
			}
			else {
				SET_ERROR_LOAD_SECTION
			}
		}
		else {
			SET_ERROR_LOAD_SECTION
		}
	}
	else {
		SET_ERROR_LOAD_SECTION
	}
	delete wb;
	delete v2F;
	delete v3F;
	delete storm;
}

void getSoundLength(EUDSettings* settings) {
	SET_NO_ERROR
	int duration = 0;

	FILE* f;
	if (fopen_s(&f, (char*)settings->inputFilePath, "rb")) {
		SET_ERROR_LOAD_FILE
		return;
	}
	bool error = false;
	ReadBuffer* rb = new ReadBuffer(f, &error);
	fclose(f);
	if (error) {
		SET_ERROR_LOAD_FILE
		delete rb;
		return;
	}

	unsigned int size = rb->getDataSize();
	unsigned char* data = rb->readArray(size, &error);
	delete rb;
	if (error) {
		free(data);
		return;
	}

	if (ENDS_WIDTH(((char*)settings->inputFilePath), ".wav")) {
		duration = getWavLengthMs((char*) data, &error);
	}
	else if (ENDS_WIDTH(((char*)settings->inputFilePath), ".ogg")) {
		duration = getOggLengthMs((char*) data, size, &error);
	}
	if (error) {
		SET_ERROR_LOAD_FILE
	}

	free(data);
	settings->outputFilePath = (char*)duration;
}

void getForces(EUDSettings* settings) {


	bool error = false;
	Storm* storm = new Storm(&error);

	SET_NO_ERROR
		settings->outputFilePath = nullptr;

	MapFile* v3F = storm->readSCX((char*)settings->inputFilePath, &error);
	if (error) {
		SET_ERROR_LOAD_FILE
			delete v3F;
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK();
	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE
			LOG("QCHK", "Failed to load CHK from file \"%s\"", (char*) settings->inputFilePath);
		return;
	}
	Section_OWNR* forc = (Section_OWNR*)v3->getSection("OWNR");
	if (forc == nullptr) {
		SET_ERROR_LOAD_SECTION
			LOG("QCHK", "Failed to load CHK from file \"%s\"", (char*)settings->inputFilePath);
		return;
	}
	GET_CLONED_DATA(result, char, forc->data, 12, { SET_ERROR_PROCESS error = true; });
	if (!error) {
		settings->outputFilePath = result;
	}
	delete v3F;
	delete storm;
	settings->TimeLockMessage = nullptr;

}

void getRecalculatedData(EUDSettings* settings) {


	bool error = false;
	Storm* storm = new Storm(&error);

	SET_NO_ERROR
	settings->outputFilePath = nullptr;

	MapFile* v3F = storm->readSCX((char*)settings->inputFilePath, &error);
	if (error) {
		SET_ERROR_LOAD_FILE
		delete v3F;
		delete storm;
		return;
	}

	CHK* v3 = v3F->getCHK();
	if (v3 == nullptr) {
		SET_ERROR_LOAD_FILE
			LOG("QCHK", "Failed to load CHK from file \"%s\"", (char*)settings->inputFilePath);
		return;
	}
	Section_UNIx* UNIx = (Section_UNIx*)v3->getSection("UNIx");
	if (UNIx== nullptr) {
		SET_ERROR_LOAD_SECTION
		LOG("QCHK", "Failed to load CHK from file \"%s\"", (char*)settings->inputFilePath);
		return;
	}
	WriteBuffer wb;
	GET_CLONED_DATA(pre, UnitSettings, UNIx->data, 1, { SET_ERROR_PROCESS error = true; delete v3F; delete storm; return; });
	wb.writeArray((unsigned char*)pre, sizeof(UnitSettings), &error);
	delete pre;
	if (!fix13_RecalculateHPAndDamage(nullptr, v3, settings)) {
		SET_ERROR_PROCESS
		delete v3F;
		delete storm;
		return;
	}
	
	
	GET_CLONED_DATA(post, UnitSettings, UNIx->data, 1, { SET_ERROR_PROCESS error = true; delete v3F; delete storm; return; });
	wb.writeArray((unsigned char*)post, sizeof(UnitSettings), &error);
	delete post;
	if (error) {
		SET_ERROR_PROCESS
		delete v3F;
		delete storm;
		return;
	}
	char* result;
	unsigned int length;
	wb.getWrittenData((unsigned char**) (&result), &length);

	GET_CLONED_DATA(data, char, result, length, { error = true; });

	settings->outputFilePath = data;
	delete v3F;
	delete storm;
	settings->TimeLockMessage = nullptr;

}

LIBRARY_API void __cdecl realize(EUDSettings* settings) {
	fprintf(stderr, "QCHK Entry\n");
	LOG("QCHK", "Received request to perform action %d", settings->action);
	if (settings->action == 1) { // The main action
		processMap(settings);
	}
	else if (settings->action == 2) { // Load WAVs
		loadWavs(settings);
	}
	else if (settings->action == 3) { // Free load wav result
		freeWavs(settings);
	}
	else if (settings->action == 4) { // Get sound length
		getSoundLength(settings);
	}
	else if (settings->action == 5) { // Get forces
		getForces(settings);
	}
	else if (settings->action == 6) { // Get recalculated data
		getRecalculatedData(settings);
	}
#ifndef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtDumpMemoryLeaks();
#endif
}

