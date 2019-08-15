#pragma once
#include "common.h"
#include "CHK.h"
#include <ctime>
#include "EUDSettings.h"
#include "SoundCollection.h"
#include "wav.h"
#include "math.h"

bool fix0_fixFog(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_disableHyperTriggers(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_disableDefaultAlliances(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_fixColors(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix2_relocateStrings(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_ResetUnusedUnitsToTheirDefaultValues(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix1_muteSounds(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix3_FixDeathsLabel(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix5_DisableLeaderboard(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix6_FixCenteringViewAtBeginning(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix7_DisableVisor(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix8_DisableTouchRevive(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix9_AddTimeLockTriggers(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix10_CopyForceNames(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix11_CopyUnitProperties(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix12_DisableBarrier(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix13_RemapLocations(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix14_AddElapsedTimeToAllConditions(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix15_ImportWav(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix16_DisableEndGames(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix17_RecalculateHPAndDamage(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix18_CopySections(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix19_CopyScenarionNameAndDescription(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix20_CopyTriggersAndBriefing(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix21_CopyUnitSettings(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix22_RelocateSTREUDSection(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix4_AddInitialObjectives(CHK* v2, CHK* v3, EUDSettings* settings);