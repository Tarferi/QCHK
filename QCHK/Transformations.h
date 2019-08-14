#pragma once
#include "common.h"
#include "CHK.h"
#include <ctime>
#include "EUDSettings.h"
#include "SoundCollection.h"
#include "wav.h"
#include "math.h"

bool fix0_disableHyperTriggers(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_disableDefaultAlliances(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_fixColors(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_relocateStrings(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_ResetUnusedUnitsToTheirDefaultValues(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix0_muteSounds(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix1_FixDeathsLabel(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix1_DisableLeaderboard(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix2_FixCenteringViewAtBeginning(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix3_DisableVisor(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix4_DisableTouchRevive(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix5_AddTimeLockTriggers(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix6_CopyForceNames(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix7_CopyUnitProperties(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix8_DisableBarrier(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix9_RemapLocations(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix10_AddElapsedTimeToAllConditions(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix11_ImportWav(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix12_DisableEndGames(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix13_RecalculateHPAndDamage(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix14_CopySections(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix15_CopyScenarionNameAndDescription(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix16_CopyTriggersAndBriefing(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix17_CopyUnitSettings(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix18_RelocateSTREUDSection(CHK* v2, CHK* v3, EUDSettings* settings);

bool fix19_AddInitialObjectives(CHK* v2, CHK* v3, EUDSettings* settings);