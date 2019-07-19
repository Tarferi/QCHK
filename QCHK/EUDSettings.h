#pragma once

#ifndef _DEBUG
#define EUDSettings PeniStruct
#endif

#define SET_ERROR(flag){unsigned int val = settings->result; val |= (1 << flag); settings->result = val;}

#define SET_ERROR_LOAD_FILE SET_ERROR(1)
#define SET_ERROR_LOAD_SECTION SET_ERROR(2)
#define SET_ERROR_PROCESS SET_ERROR(3)
#define SET_NO_ERROR settings->result = 0;

#pragma pack(push, 1)
 _declspec(align(1)) // Disable alignment
struct EUDSettings {

	uint8 action;

	bool8 addTouchRevive;
	bool8 useDefaultGunShot;
	bool8 useDefaultBackgroundMusic;
	bool8 enableVisor;
	bool8 enableBarrier;
	bool8 addLeaderboard;
	bool8 addTimeLock;
	
	bool8 useSanctuaryColors;

	bool8 recalculateHPAndDamage;
	bool8 muteUnits;

	void* GunShotWavFilePath;
	void* VisorUsageFilePath;
	void* BackgroundWavFilePath;

	void* TimeLockMessage;
	void* TimeLockFrom;
	void* TimeLockTo;
	void* inputFilePath;
	void* outputFilePath;

	uint16 EMPDamage;
	uint32 result;

	void* preferredUnitSettings;
	void* weaponsToIgnoreArmors;

};
#pragma pack(pop)