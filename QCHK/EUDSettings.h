#pragma once

#ifndef _DEBUG
#define EUDSettings PeniStruct
#endif

#define SET_ERROR(flag){unsigned int val = settings->result; val |= (1 << flag); settings->result = val;}

#define SET_ERROR_LOAD_FILE SET_ERROR(1)
#define SET_ERROR_LOAD_SECTION SET_ERROR(2)
#define SET_ERROR_PROCESS SET_ERROR(3)
#define SET_NO_ERROR settings->result = 0;


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

	unsigned int result;

} EUDSettings;