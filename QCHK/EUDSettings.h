#pragma once

#ifndef _DEBUG
#define EUDSettings PeniStruct
#endif

#define SET_ERROR(flag){unsigned int val = (unsigned int) settings->TimeLockMessage; val |= (1 << flag); settings->TimeLockMessage = (char*)val;}

#define SET_ERROR_LOAD_FILE SET_ERROR(1)
#define SET_ERROR_LOAD_SECTION SET_ERROR(2)
#define SET_ERROR_PROCESS SET_ERROR(3)
#define SET_NO_ERROR settings->TimeLockMessage = nullptr;


typedef struct EUDSettings {

	unsigned char action;

	bool addTouchRevive;
	bool useDefaultGunShot;
	bool useDefaultBackgroundMusic;
	bool enableVisor;
	bool enableBarrier;
	bool addLeaderboard;
	bool addTimeLock;
	
	bool useSanctuaryColors;

	char* GunShotWavFilePath;
	char* VisorUsageFilePath;
	char* BackgroundWavFilePath;

	char* TimeLockMessage;
	char* TimeLockFrom;
	char* TimeLockTo;
	char* inputFilePath;
	char* outputFilePath;

	unsigned short EMPDamage;

	bool verbose;

} EUDSettings;