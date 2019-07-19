#include "Transformations.h"

#define GET_SECT(type, name, source, sourceName) type* name = (type*) source->getSection(sourceName);if(!name){return false;}

bool fix0_disableDefaultAlliances(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	v2TRIG->triggers.get(293)->conditions[0].ConditionType = 23;
	for (unsigned int i = 224; i < 232; i++) {
		Trigger* trig = v2TRIG->triggers.get(i);
		if (i != 226) {
			trig->conditions[0].ConditionType = 23;
		}
		else {
			trig->actions[0].Player = 17; // Force 2 to all as well
			trig->actions[0].UnitType = 1; // Ally

			trig->actions[1].Player = 18; // Force 1 to all as well
			trig->actions[1].UnitType = 1; // Ally
		}
	}
	return true;
}

bool fix0_fixColors(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_COLR, v3COLR, v3, "COLR");
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	Trigger* trigger = v2TRIG->triggers.get(237);
	for (unsigned int i = 4; i < 22; i++) { // Disable color remapping actions
		trigger->actions[i].Flags |= 2;
	}
	return true;
}

#define REMAP_STR(index, triggerindex, actionindex) {\
GET_CLONED_STRING(nstr, v2STR->getRawString(index+1), {stringsToRelocate.freeItems(); return false;});\
stringsToRelocate.append(nstr);\
actionsUsingThoseStrings.append(&(((Trigger*)(v2TRIG->triggers[triggerindex]))->actions[actionindex]));\
}\

#define REMAP_XSTR(origStr, triggerindex, actionindex) {\
GET_CLONED_STRING(nstr, origStr, {stringsToRelocate.freeItems(); return false;});\
stringsToRelocate.append(nstr);\
actionsUsingThoseStrings.append(&(((Trigger*)(v2TRIG->triggers[triggerindex]))->actions[actionindex]));\
}\

bool fix0_relocateStrings(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	Array<char*> stringsToRelocate;
	Array<Action*> actionsUsingThoseStrings;
	if (settings->enableBarrier) {
		REMAP_STR(311, 249, 0); // Barrier charge
		LOG("STR RELOCATOR", "Relocating barrier charged message")
	}
	if (settings->enableVisor) {
		REMAP_STR(314, 321, 0); // Visor charge
		LOG("STR RELOCATOR", "Relocating visor charged message")
	}

	if (settings->enableVisor && settings->enableBarrier) {
		REMAP_STR(309, 313, 0); // Visor available barrier available
		REMAP_STR(316, 314, 0); // Barrier available visor disabled
		REMAP_STR(315, 315, 0); // Barrier disabled visor available
		REMAP_STR(310, 316, 0); // Barrier disabled visor disabled
		LOG("STR RELOCATOR", "Relocating barrier and barrier available/disabled messages")
	}
	else if (settings->enableVisor && !settings->enableBarrier) {
		REMAP_XSTR("\x04" "[P] " "\x1f" "Automatic Visor available", 313, 0); // Visor available barrier available
		REMAP_XSTR("\x04" "[P] " "\x08" "Automatic Visor disabled", 314, 0); // Barrier available visor disabled
		REMAP_XSTR("\x04" "[P] " "\x1f" "Automatic Visor available", 315, 0); // Barrier disabled visor available
		REMAP_XSTR("\x04" "[P] " "\x08" "Automatic Visor disabled", 316, 0); // Barrier disabled visor disabled
		LOG("STR RELOCATOR", "Relocating only visor messages")
	}
	else if (!settings->enableVisor && settings->enableBarrier) {
		REMAP_XSTR("\x04" "[F] " "\x1f" "Barrier available", 313, 0); // Visor available barrier available
		REMAP_XSTR("\x04" "[F] " "\x1f" "Barrier available", 314, 0); // Barrier available visor disabled
		REMAP_XSTR("\x04" "[F] " "\x08" "Barrier disabled", 315, 0); // Barrier disabled visor available
		REMAP_XSTR("\x04" "[F] " "\x08" "Barrier disabled", 316, 0); // Barrier disabled visor disabled
		LOG("STR RELOCATOR", "Relocating only barrier messages")
	} else { // Disable visor and barrier ? remap nothing, disable actions
		v2TRIG->triggers.get(313)->actions[0].Flags |= 2; // Disable barrier and visor messages
		v2TRIG->triggers.get(314)->actions[0].Flags |= 2; // Disable barrier and visor messages
		v2TRIG->triggers.get(315)->actions[0].Flags |= 2; // Disable barrier and visor messages
		v2TRIG->triggers.get(316)->actions[0].Flags |= 2; // Disable barrier and visor messages

		v2TRIG->triggers.get(313)->actions[1].Flags |= 2; // Disable barrier and visor sounds
		v2TRIG->triggers.get(314)->actions[1].Flags |= 2; // Disable barrier and visor sounds
		v2TRIG->triggers.get(315)->actions[1].Flags |= 2; // Disable barrier and visor sounds
		v2TRIG->triggers.get(316)->actions[3].Flags |= 2; // Disable barrier and visor sounds
	}
	if (settings->addLeaderboard) {
		REMAP_STR(5, 234, 0);
		REMAP_XSTR("\x03" "D" "\x04" "eaths", 240, 0);
		LOG("STR RELOCATOR", "Fixing deaths leaderboard title");
	}

	// Remap those strings
	for (unsigned int i = 0; i < stringsToRelocate.getSize(); i++) {
		char* string = stringsToRelocate.get(i);
		Action* action = actionsUsingThoseStrings.get(i);
		action->TriggerText = v2STR->getNewStringIndex(string);
		free(string);
	}
	return true;
}

bool fix1_FixDeathsLabel(CHK* v2, CHK* v3, EUDSettings* settings) {
	/*
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_STR_, v2STR, v2, "STR ");

	v2STR->setRawString(6, "\x08" "K" "\x04" "ills");
	Trigger* trig = v2TRIG->triggers[240];
	trig->actions[0].TriggerText = v2STR->getNewStringIndex("\x03" "D" "\x04" "eaths");
	*/
	return true;
}

bool fix1_DisableLeaderboard(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	v2TRIG->triggers.get(234)->conditions[0].ConditionType = 23;
	v2TRIG->triggers.get(240)->conditions[0].ConditionType = 23;
	LOG("DISABLE LEADERBOARD", "disabling triggers %d and %d", 234, 240)
	return true;
}

bool fix2_FixCenteringViewAtBeginning(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	v2TRIG->triggers.get(306)->conditions[0].ConditionType = 23;
	LOG("DISABLE CENTERVIEW", "disabling trigger %d", 306);

	LOG("DISABLE AI", "Disabling AI");
	v2TRIG->triggers.get(289)->conditions[0].ConditionType = 23;
	v2TRIG->triggers.get(293)->conditions[0].ConditionType = 23;

	v2TRIG->triggers.get(235)->actions[27].Flags |= 2;
	return true;
}

bool fix3_DisableVisor(CHK* v2, CHK* v3, EUDSettings* settings) {
	
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	for (unsigned int i = 317; i < 320; i++) {
		v2TRIG->triggers.get(i)->conditions[0].ConditionType = 23;
		LOG("DISABLE VISOR", "disabling trigger %d", i)
	}

	v2TRIG->triggers.get(312)->actions[2].Flags |= 2; // Disable visor usage sound
	v2TRIG->triggers.get(321)->actions[0].Flags |= 2;
	LOG("DISABLE VISOR", "disabling sound effect in action %d of trigger %d", 2, 312)
	LOG("DISABLE VISOR", "disabling sound effect in action %d of trigger %d", 0, 321)
	return true;
}

bool fix4_DisableTouchRevive(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	for (unsigned int i = 256; i < 289; i++) {
		v2TRIG->triggers.get(i)->conditions[0].ConditionType = 23;
		LOG("TOUCH REVIVE", "disabling trigger %d", i)
	}
	for (unsigned int i = 250; i < 256; i++) {
		v2TRIG->triggers.get(i)->actions[0].Flags |= 2;
		LOG("TOUCH REVIVE", "disabling trigger %d", i)
	}
	return true;
}

bool replaceTimeFragment(char* string, char* lookup, char* replacement) {
	char* strPos = strstr(string, lookup);
	if (strPos != nullptr) {
		unsigned int offset = strPos - string;
		unsigned int stringLength = strlen(string);
		unsigned int lookupLength = strlen(lookup);
		unsigned int replacementLength = strlen(replacement);
		unsigned int offsetLength = stringLength - offset;
		memcpy(strPos, replacement, replacementLength);
		memcpy(strPos + replacementLength, strPos + lookupLength, offsetLength - (lookupLength - replacementLength));
		return true;
	}
	return false;
}

bool replaceTime(char* string, char type, unsigned long timeX) {
	unsigned char M_str[5];
	unsigned char D_str[5];
	unsigned char Y_str[7];
	unsigned char H_str[5];
	unsigned char m_str[5];
	unsigned char S_str[5];

	unsigned short M = 0;
	unsigned short D = 0;
	unsigned short Y = 0;
	unsigned short H = 0;
	unsigned short m = 0;
	unsigned short S = 0;

	time_t tm = timeX;
	std::tm time;
	localtime_s(&time, &tm);
	M = time.tm_mon + 1;
	D = time.tm_mday;
	Y = 1900 + time.tm_year;
	H = time.tm_hour;
	m = time.tm_min;
	S = time.tm_sec;

	sprintf_s((char*)M_str, 5, "%02d", M);
	sprintf_s((char*)D_str, 5, "%02d", D);
	sprintf_s((char*)Y_str, 7, "%04d", Y);
	sprintf_s((char*)H_str, 5, "%02d", H);
	sprintf_s((char*)m_str, 5, "%02d", m);
	sprintf_s((char*)S_str, 5, "%02d", S);

	GET_CLONED_STRING(M_LOOK, "%MMX%", { return false; });
	GET_CLONED_STRING(D_LOOK, "%DDX%", { free(M_LOOK); return false; });
	GET_CLONED_STRING(Y_LOOK, "%YYYYX%", { free(D_LOOK); free(M_LOOK); return false; });
	GET_CLONED_STRING(H_LOOK, "%HHX%", { free(Y_LOOK); free(D_LOOK); free(M_LOOK); return false; });
	GET_CLONED_STRING(m_LOOK, "%mmX%", { free(m_LOOK); free(Y_LOOK); free(D_LOOK); free(M_LOOK); return false; });
	GET_CLONED_STRING(S_LOOK, "%SSX%", { free(S_LOOK); free(m_LOOK); free(Y_LOOK); free(D_LOOK); free(M_LOOK); return false; });

	M_LOOK[3] = type;
	D_LOOK[3] = type;
	Y_LOOK[5] = type;
	H_LOOK[3] = type;
	m_LOOK[3] = type;
	S_LOOK[3] = type;

	bool repl = false;
	bool error = false;
	repl |= replaceTimeFragment(string, M_LOOK, (char*)M_str);
	repl |= replaceTimeFragment(string, D_LOOK, (char*)D_str);
	repl |= replaceTimeFragment(string, Y_LOOK, (char*)Y_str);
	repl |= replaceTimeFragment(string, H_LOOK, (char*)H_str);
	repl |= replaceTimeFragment(string, m_LOOK, (char*)m_str);
	repl |= replaceTimeFragment(string, S_LOOK, (char*)S_str);


	free(M_LOOK);
	free(D_LOOK);
	free(Y_LOOK);
	free(H_LOOK);
	free(m_LOOK);
	free(S_LOOK);

	return repl;
}

char* transformUserString(char* string, bool* error) {
	GET_CLONED_STRING(src, "<01>", { *error = true; return 0; });
	GET_CLONED_STRING(dst, "\x01", { free(src); *error = true; return 0; });

	for (unsigned short i = 0; i <= 0x1f; i++) {
		dst[0] = (char) i;
		dst[1] = (char) 0;

		//char hexDigitL[] = { '0', '1', '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , 'a' , 'b' , 'c' , 'd', 'e', 'f' };
		//char hexDigitH[] = { '0', '1', '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , 'a' , 'b' , 'c' , 'd', 'e', 'f' };

		sprintf_s(src, 5, "<%02x>", i);
		while (replaceTimeFragment(string, src, dst)) {};

		sprintf_s(src, 5, "<%02X>", i);
		while (replaceTimeFragment(string, src, dst)) {};

	}

	replaceTimeFragment(string, (char*) "<01>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<02>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<03>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<04>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<05>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<06>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<07>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<08>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<09>", (char*)"\x01");
	replaceTimeFragment(string, (char*) "<0A>", (char*)"\x01");

	free(src);
	free(dst);

	return string;
}

bool addTimeLockTrigger(Section_TRIG* v2TRIG, unsigned char comparator, unsigned short stringIndex, unsigned int time) {
	MALLOC_N(newTrig, Trigger, 1, { return false; });
	memset(newTrig, 0, sizeof(Trigger));
	newTrig->flags = 0;
	newTrig->players[17] = 1; // All players
	newTrig->conditions[0].ConditionType = 15; // Deaths
	newTrig->conditions[0].groupNumber = 334581; // Seconds
	newTrig->conditions[0].Comparision = comparator; // At most / at least
	newTrig->conditions[0].Quantifier = time; // Max allowed time
	
	newTrig->actions[0].ActionType = 9; // Display text message
	newTrig->actions[0].Flags = 4;
	newTrig->actions[0].TriggerText = stringIndex; // Display text message

	newTrig->actions[1].ActionType = 4; // Wait
	newTrig->actions[1].Time = 5000; // Display text message

	newTrig->actions[2].ActionType = 2; // Defeat

	if (v2TRIG->triggers.append(newTrig)) {
		return true;
	}
	else {
		return false;
	}
}

unsigned long getTimeDiff(char* str, bool* error) {
	unsigned long minute = 60;
	unsigned long hour = 60 * minute;
	unsigned long day = 24 * hour;
	unsigned long month = 30 * day; // Don't judge
	unsigned long year = 365 * day; // The extra day doesn't really matter

	int len = strlen(str);
	GET_CLONED_STRING_LEN(xstr, str, len + 3, { *error = true; return 0; }); // Extra space
	xstr[len] = ':';
	xstr[len + 1] = 0;
	char* toFree = xstr;

#define TIMEHELP(i) ends[i] = strstr(xstr, ":");ends[i][0] = (char)0;ends[i] = xstr;xstr = xstr + (strlen(ends[i])) + 1; iends[i] = atoi(ends[i]);

	char* ends[6];
	int iends[6];

	TIMEHELP(0);
	TIMEHELP(1);
	TIMEHELP(2);
	TIMEHELP(3);
	TIMEHELP(4);
	TIMEHELP(5);

	free(toFree);

	unsigned long result = iends[5]; // Seconds
	result += iends[4] * minute; // Minutes
	result += iends[3] * hour; // Hours
	result += iends[2] * day; // Days
	result += iends[1] * month; // Months
	result += iends[0] * year; // Years
	LOG("TIMELOCK", "Decoded string \"%s\", to %d years, %d months, %d days, %d hours, %d minutes and %d seconds", str, iends[0], iends[1], iends[2], iends[3], iends[4], iends[5])
	return result;
}

bool fix5_AddTimeLockTriggers(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");

	GET_SECT(Section_TRIG, v3MBRF, v3, "MBRF");
	GET_SECT(Section_SPRP, v3SPRP, v3, "SPRP");

	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	unsigned long now = (unsigned long) std::time(nullptr);
	
	bool error = false;

	unsigned int timeFrom = getTimeDiff((char*)settings->TimeLockFrom, &error);
	if (error) {
		return false;
	}
	unsigned int timeTo = getTimeDiff((char*)settings->TimeLockTo, &error);
	if (error) {
		return false;
	}

	unsigned long lockFrom = now - timeFrom;
	unsigned long lockTo = now + timeTo;

	//GET_CLONED_STRING(userString, "<02>This map is time-locked from <03>%DDF%.%MMF%.%YYYYF% %HHF%:%mmF% <02>to <03>%DDT%.%MMT%.%YYYYT% %HHT%:%mmT% <02>and therefore unplayable now!");
	GET_CLONED_STRING(userString, ((char*)settings->TimeLockMessage), { return false; });
	replaceTime(userString, 'F', lockFrom);
	replaceTime(userString, 'T', lockTo);
	LOG("TIMELOCK", "Now is %d, lock begin is %d and lock and is %d, lock message is %s", now, lockFrom, lockTo, userString);
	transformUserString(userString, &error);
	if (error) {
		free(userString);
		return false;
	}
	unsigned short string = v2STR->getNewStringIndex(userString);

	free(userString);

	unsigned char AT_LEAST = 0;
	unsigned char AT_MOST = 1;

	addTimeLockTrigger(v2TRIG, AT_LEAST, string, lockTo);
	addTimeLockTrigger(v2TRIG, AT_MOST, string, lockFrom);
	
	for (unsigned int i = 0; i < v3TRIG->triggers.getSize(); i++) {
		Trigger* trigger = v3TRIG->triggers.get(i);
		for (unsigned int o = 0; o < 64; o++) {
			Action* action = &(trigger->actions[o]);
			if (action->TriggerText != 0) {
				char* string = v3STR->getRawString(action->TriggerText);
				if (replaceTime(string, 'F', lockFrom) || replaceTime(string, 'T', lockTo)) {
					v3STR->setRawString(action->TriggerText, string);
					LOG("TIMELOCK", "Replacing string index %d found in action %d of trigger %d to \"%s\"", action->TriggerText, o, i, string);
				}
			}
		}
	}
	char* scenarioDescription = v3STR->getRawString(v3SPRP->str_scenarioDescription);
	if (replaceTime(scenarioDescription, 'F', lockFrom) || replaceTime(scenarioDescription, 'T', lockTo)) {
		GET_CLONED_STRING(scenarioDescription_cloned, scenarioDescription, { return false; });
		v3STR->setRawString(v3SPRP->str_scenarioDescription, scenarioDescription_cloned);
		LOG("TIMELOCK", "Replacing scenarion description to \"%s\"", scenarioDescription);
		free(scenarioDescription_cloned);
	}
	return true;
}

bool fix6_CopyForceNames(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_FORC, v2FORC, v2, "FORC");
	GET_SECT(Section_FORC, v3FORC, v3, "FORC");

	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	for (unsigned int i = 0; i < 4; i++) {
		if (v3FORC->data->str_names[i] != 0) {
			v3FORC->data->str_names[i] = v2STR->getNewStringIndex(v3STR->getRawString(v3FORC->data->str_names[i]));
		}
	}
	return true;
}

bool fix7_CopyUnitProperties(CHK * v2, CHK * v3, EUDSettings * settings) {
	GET_SECT(Section_UPRP, v2Prop, v2, "UPRP");
	GET_SECT(Section_UPRP, v3Prop, v3, "UPRP");

	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");

	unsigned char* v2PropRawData = v2Prop->data;
	unsigned char* v3PropRawData = v3Prop->data;

	bool usedV2Structures[64];
	char v3StructureRemapping[64];
	for (unsigned int structIndex = 0; structIndex < 64; structIndex++) {
		usedV2Structures[structIndex] = false;
		v3StructureRemapping[structIndex] = -1;
	}

	// Find used slots in native triggers
	for (unsigned int triggerIndex = 0; triggerIndex < v2TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = v2TRIG->triggers[triggerIndex];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* act = &(trigger->actions[actionIndex]);
			if (act->ActionType == 11) { // Create units with properties
				unsigned int propID = act->Group;
				if (propID != 0 && propID >= 64) {
					LOG("PROPERTIES REMAPPER", "Action %d of trigger %d uses properties of index %d, which is invalid (>= 64)", actionIndex, triggerIndex, propID);
				}
				else {
					propID--; // 1 based
					if (!usedV2Structures[propID]) {
						LOG("PROPERTIES REMAPPER", "Native uses properties at index %d", propID);
						usedV2Structures[propID] = true;
					}
				}
			}
		}
	}

	// Count free slots
	unsigned int availableSlots = 0;
	for (unsigned int structIndex = 0; structIndex < 64; structIndex++) {
		if (usedV2Structures[structIndex]) {
			availableSlots++;
		}
	}
	LOG("PROPERTIES REMAPPER", "Native uses %d slots of total 64", 64 - availableSlots);

	// Remap v3 actions to free slots
	int lastUsedSlot = 0;
	for (unsigned int triggerIndex = 0; triggerIndex < v3TRIG->triggers.getSize(); triggerIndex++) {
		Trigger* trigger = v3TRIG->triggers[triggerIndex];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* act = &(trigger->actions[actionIndex]);
			if (act->ActionType == 11) { // Create units with properties
				unsigned int propID = act->Group;
				if (propID != 0 && propID >= 64) {
					LOG("PROPERTIES REMAPPER", "Action %d of trigger %d in imported map uses properties of index %d", actionIndex, triggerIndex, propID);
				}
				else { // Find free slot to use
					propID--;
					char remappingIndex = v3StructureRemapping[propID];
					if (remappingIndex == -1) { // Not remapped yet
						while (lastUsedSlot < 64) {
							if (!usedV2Structures[lastUsedSlot]) {
								break;
							}
							lastUsedSlot++;
						};
						if (lastUsedSlot >= 64) {
							LOG("PROPERTIES REMAPPER", "Not enough properties slots available");
							return false;
						}

						// Set remapping
						v3StructureRemapping[propID] = lastUsedSlot;
						usedV2Structures[lastUsedSlot] = true;
						act->Group = lastUsedSlot + 1;

						// Copy raw data to that slot
						memcpy(v2PropRawData + (64 * lastUsedSlot), v3PropRawData + (64 * propID), 64);
						LOG("PROPERTIES REMAPPER", "Remapping properties used in action %d of trigger %d in imported map uses properties of index %d to new index %d and copying data", actionIndex, triggerIndex, propID, lastUsedSlot);
					}
					else { // Remapping is known from before
						act->Group = remappingIndex + 1;
						LOG("PROPERTIES REMAPPER", "Remapping properties used in action %d of trigger %d in imported map uses properties of index %d to new index %d", actionIndex, triggerIndex, propID, remappingIndex);
					}
				}
			}
		}
	}
	return true;
}

void RemapLocation(unsigned int nextRemapLocationIndex, unsigned int* nextRemapLocationIndexPtr, Array<unsigned int>* remapedLocations, unsigned int *freeLocationRangesBeginPtr, unsigned int *freeLocationRangesEndPtr, unsigned int *locationIndexPtr, unsigned int freeLocationsRangeLength) {
	unsigned int locationIndex = *locationIndexPtr;

	// If exists, find it
	for (unsigned int i = 0; i < remapedLocations->getSize(); i += 2) {
		unsigned int originalLocation = remapedLocations->get(i);
		if (originalLocation == locationIndex) {
			unsigned int remapedLocation = remapedLocations->get(i + 1);
			*locationIndexPtr = remapedLocation;
			return;
		}
	}

	// Otherwise insert it
	remapedLocations->append(locationIndex);
	remapedLocations->append(nextRemapLocationIndex);
	*locationIndexPtr = nextRemapLocationIndex;

	// Update next range 
	for (unsigned int i = 0; i < freeLocationsRangeLength - 1; i++) {
		unsigned int rangeEnd = freeLocationRangesEndPtr[i];
		if (nextRemapLocationIndex == rangeEnd) {
			nextRemapLocationIndex = freeLocationRangesBeginPtr[i + 1] - 1;
			break;
		}
	}
	*nextRemapLocationIndexPtr = nextRemapLocationIndex + 1;
}

bool fix8_DisableBarrier(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	v2STR->data[176244] = 51; // For some reason this disables the healing ability

	v2TRIG->triggers.get(249)->actions[1].Flags |= 2; // Disable barrier availability sound
	v2TRIG->triggers.get(249)->actions[3].Flags |= 2; // Disable barrier availability message

	LOG("DISABLE BARRIER", "disabling sound effect in action %d of trigger %d", 1, 249)
	LOG("DISABLE BARRIER", "disabling message in action %d of trigger %d", 3, 249)
	return true;
}

bool fix9_RemapLocations(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_MRGN, v2MRGN, v2, "MRGN");
	GET_SECT(Section_MRGN, v3MRGN, v3, "MRGN");

	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	unsigned int freeLocationRangesBegs[] = { 43,  66, 122, 164 };
	unsigned int freeLocationRangesEnds[] = { 62, 118, 135, 237 };


	unsigned int freeLocationRangesLength = 4;
	unsigned char remapConditionNeededI[] = { 3, 7, 17 };
	unsigned char remapActionNeededI[] = { 7, 10, 11, 16, 18, 23, 25, 28, 34, 38, 39, 42, 43, 44, 46, 48, 49, 50, 51, 52, 53 };

	// Use binary boolean fields
	bool remapConditionNeeded[256];
	bool remapActionNeeded[256];
	memset(remapConditionNeeded, false, 256);
	memset(remapActionNeeded, false, 256);
	for (unsigned int i = 0; i < sizeof(remapConditionNeededI) / sizeof(char); i++) {
		remapConditionNeeded[remapConditionNeededI[i]] = true;
	}
	for (unsigned int i = 0; i < sizeof(remapActionNeededI) / sizeof(char); i++) {
		remapActionNeeded[remapActionNeededI[i]] = true;
	}

	Array<unsigned int> remapedLocations;
	unsigned int nextRemapLocationIndex = freeLocationRangesBegs[0];

	remapedLocations.append(63);
	remapedLocations.append(63);


	// Collect triggers that use locations and remap those locations
	for (unsigned int i = 0; i < v3TRIG->triggers.getSize(); i++) {
		Trigger* trigger = v3TRIG->triggers[i];
		for (unsigned int o = 0; o < 16; o++) {
			Condition* cond = &(trigger->conditions[o]);

			// Check if condition should be relocated
			if (cond->ConditionType >= 0 && cond->ConditionType < 256) {
				if (remapConditionNeeded[cond->ConditionType] && cond->locationNumber > 0 && cond->locationNumber != 64) {
					unsigned int locationNumber = cond->locationNumber;
					locationNumber--;
					LOG("LOCATION REMAPPER", "Found location %d in trigger %d condition %d that needs remapping", locationNumber, i, o);
					RemapLocation(nextRemapLocationIndex, &nextRemapLocationIndex, &remapedLocations, freeLocationRangesBegs, freeLocationRangesEnds, &locationNumber, freeLocationRangesLength);
					cond->locationNumber = locationNumber + 1;
				}
			}
		}
		for (unsigned int o = 0; o < 64; o++) {
			Action* action = &(trigger->actions[o]);

			// Check if condition should be relocated
			if (action->ActionType >= 0 && action->ActionType < 256) {
				if (remapActionNeededI[action->ActionType] && action->SourceLocation > 0 && action->SourceLocation != 64) {
					unsigned int locationNumber = action->SourceLocation;
					locationNumber--;
					LOG("LOCATION REMAPPER", "Found location %d in trigger %d action %d that needs remapping", locationNumber, i, o);
					RemapLocation(nextRemapLocationIndex, &nextRemapLocationIndex, &remapedLocations, freeLocationRangesBegs, freeLocationRangesEnds, &locationNumber, freeLocationRangesLength);
					action->SourceLocation = locationNumber + 1;
				}
			}
		}
	}

	// Perform relocation within section
	for (unsigned int i = 0; i < remapedLocations.getSize(); i += 2) {
		unsigned int originalLocation = remapedLocations.get(i);
		unsigned int remapedLocation = remapedLocations.get(i + 1);
		Location* v2L = v2MRGN->locations.get(remapedLocation);
		Location* v3L = v3MRGN->locations.get(originalLocation);
		LOG("LOCATION REMAPPER", "Remapped location %d (%d, %d, %d, %d) will be remapped into location %d", originalLocation, v3L->left, v3L->top, v3L->right, v3L->bottom, remapedLocation);
		v2L->bottom = v3L->bottom;
		v2L->elevation = v3L->elevation;
		v2L->left = v3L->left;
		v2L->right = v3L->right;
		v2L->top = v3L->top;
		v2L->str_description = 0;
	}

	// Calculate triggers
	unsigned int locationsPerTrigger = 3; // Reserve 1 slot for counter operation, (5 * 12) + 1 actions per trigger
	unsigned int totalRemappedLocations = remapedLocations.getSize() / 2;
	unsigned int totalTriggers = totalRemappedLocations / locationsPerTrigger;
	totalTriggers += totalTriggers*locationsPerTrigger == totalRemappedLocations ? 0 : 1;
	
	LOG("LOCATION REMAPPER", "Remapping %d locations in %d triggers (%d locations per trigger", remapedLocations.getSize()/2, totalTriggers, locationsPerTrigger);

	unsigned int remapCounterUnitID = 220;

	// Make the triggers
	for (unsigned int triggerIndex = 0; triggerIndex < totalTriggers; triggerIndex++) {
		MALLOC_N(trigger, Trigger, 1, { return false; });
		bool isFirstTrigger = triggerIndex == 0;
		memset(trigger, 0, sizeof(Trigger));

		trigger->players[17] = 1;  // All players

		if (isFirstTrigger) { // First trigger begind link of remapping
			trigger->conditions[0].ConditionType = 12; // Elapsed time
			trigger->conditions[0].Comparision = 0; // At least
			trigger->conditions[0].Quantifier = 1; // 1 Second
		}
		else { // Every other trigger expects > 1 counter value
			trigger->conditions[0].ConditionType = 15; // Deaths
			trigger->conditions[0].groupNumber = 6; // Player 7
			trigger->conditions[0].Comparision = 0; // At least
			trigger->conditions[0].Quantifier = 2; // Total (totalTriggers - 1) + 1 non-first triggers, first makes the first value high
			trigger->conditions[0].UnitID = remapCounterUnitID; // Mineral Cluster Type 1
		}

		unsigned int firstLocation = triggerIndex * locationsPerTrigger;
		unsigned int lastLocation = firstLocation + locationsPerTrigger;
		lastLocation = lastLocation > totalRemappedLocations ? totalRemappedLocations : lastLocation;

		unsigned int base = 3647;

		unsigned int locationIndexWithinActions = 0;
		for (unsigned int locationIndex = firstLocation; locationIndex < lastLocation; locationIndex++) {
			unsigned int originalLocationIndex = remapedLocations[locationIndex * 2];
			unsigned int remapedLocationIndex = remapedLocations[(locationIndex * 2) + 1];
			Location* location = v3MRGN->locations[originalLocationIndex];
			unsigned int locOffset = base + (remapedLocationIndex * 5);
			char* locationName = v3STR->getRawString(location->str_description);
			
			unsigned int writeData[] = { location->left, location->top, location->right, location->bottom, location->elevation };
			unsigned int writeDataLength = sizeof(writeData) / sizeof(unsigned int);
			for (unsigned int actIndex = 0; actIndex < writeDataLength; actIndex++) {

				Action* act = &(trigger->actions[locationIndexWithinActions]);

				act->ActionType = 45;  // Set deaths
				act->Player = locOffset + actIndex;  // Offset of the item
				act->UnitType = 0;  // Set deaths of marine
				act->UnitsNumber = 7;  // Set to
				act->Group = writeData[actIndex];  // Set value
				locationIndexWithinActions = locationIndexWithinActions + 1;
			}
			LOG("LOCATION RELOCATOR", "Location %d to location %d (%d, %d, %d, %d) at PID %d - %d in trigger %d", originalLocationIndex, remapedLocationIndex, location->left, location->top, location->right, location->bottom, locOffset, locOffset + 4, triggerIndex);
		}
		Action* act = &(trigger->actions[locationIndexWithinActions]);
		if (isFirstTrigger) { // First trigger sets the high avlue
			act->ActionType = 45;  // Set deaths
			act->Player = 6;  // Player 7
			act->UnitType = remapCounterUnitID;  // Mineral Cluster Type 1
			act->UnitsNumber = 7;  // Set to
			act->Group = 1 + (totalTriggers - 1); // High value, this many triggers and 1 will decrease it, last leaving it at 1
		} else { // Non-first trigger decreases the value, last decreasing to 1 begins origin triggers
			act->ActionType = 45;  // Set deaths
			act->Player = 6;  // Player 7
			act->UnitType = remapCounterUnitID;  // Mineral Cluster Type 1
			act->UnitsNumber = 9;  // Subtract
			act->Group = 1; // 1
		}
		locationIndexWithinActions++; // For further modders
		v2TRIG->triggers.append(trigger);
	}
	return true;
}

bool fix10_AddElapsedTimeToAllConditions(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");
	for (unsigned int i = 0; i < v3TRIG->triggers.getSize(); i++) {
		Trigger* trigger = v3TRIG->triggers[i];
		Array<unsigned char> empty;

		for (unsigned int conditionIndex = 0; conditionIndex < 16; conditionIndex++) {
			Condition* condition = &(trigger->conditions[conditionIndex]);
			if (condition->ConditionType == 0) { // Empty condition
				empty.append(conditionIndex);
			}
		}

		if (empty.getSize() == 0) {
			LOG("LOCATION RELOCATOR", "Cannot relocate conditions");
			return false;
		}
		unsigned int firstEmpty = empty[0];
		for (unsigned int conditionIndex = firstEmpty; conditionIndex > 0; conditionIndex--) {
			Condition tmp;
			Condition* c1 = &(trigger->conditions[conditionIndex]);
			Condition* c2 = &(trigger->conditions[conditionIndex - 1]);
			memcpy(&tmp, c1, sizeof(Condition));
			memcpy(c1, c2, sizeof(Condition));
			memcpy(c2, &tmp, sizeof(Condition));
		}
		trigger->conditions[0].ConditionType = 15; // Deaths
		trigger->conditions[0].groupNumber = 6; // Player 7
		trigger->conditions[0].Comparision = 10; // Exactly
		trigger->conditions[0].Quantifier = 1; // Bool switch that is off by default
		trigger->conditions[0].UnitID = 220; // Mineral Cluster Type 1
	}
	return true;
}

unsigned short importSound(Section_STR_* v2STR, SoundCollection* v2S, SoundCollection* v3S, SoundCollection* S, char* path, unsigned int* length, bool* error) {
	char* filePath = path + 2;
	
	if (path[0] == 'S') { // Native sound from v2S
		SoundFile* sf;
		if (v2S->getSound(filePath, &sf)) {
			*length = IS_OGG(filePath) ? getOggLengthMs(sf->contents, sf->contentsSize, error) / 1000 : getWavLengthMs(sf->contents, error) / 1000; // Sound length
			unsigned short index = S->addOrRewriteSound(v2STR, path, sf->contents, sf->contentsSize, sf->isOgg, error);
			if (*error) {
				return 0;
			}
			LOG("SOUND REMAPPING", "Remapping sound \"%s\" native data to index %d", path, index);
			return index;
		}
	}
	else if (path[0] == 'C') { // V3S sound
		SoundFile* sf;
		if (v3S->getSound(filePath, &sf)) {
			*length = IS_OGG(filePath) ? getOggLengthMs(sf->contents, sf->contentsSize, error) / 1000 : getWavLengthMs(sf->contents, error) / 1000; // Sound length
			unsigned short index = S->addOrRewriteSound(v2STR, path, sf->contents, sf->contentsSize, sf->isOgg, error);
			if (*error) {
				return 0;
			}
			LOG("SOUND REMAPPING", "Remapping sound \"%s\" another map data to index %d", path, index);
			return index;
		}
	}
	else if (path[0] == 'F') { // File system sound
		FILE* f;
		if (!fopen_s(&f, filePath, "rb")) {
			ReadBuffer rb(f, error);
			if (*error) {
				return 0;
			}
			unsigned int size = rb.getDataSize();
			char* data = (char*)rb.readArray(size, error);
			if (*error) {
				free(data);
				fclose(f);
				return 0;
			}
			unsigned short index = S->addOrRewriteSound(v2STR, path, data, size, ENDS_WIDTH(filePath, ".ogg"), error);
			if (*error) {
				free(data);
				fclose(f);
				return 0;
			}
			*length = IS_OGG(filePath) ? getOggLengthMs(data, size, error) / 1000 : getWavLengthMs(data, error) / 1000; // Sound length
			free(data);
			fclose(f);
			LOG("SOUND REMAPPING", "Remapping sound \"%s\" another map data to index %d", path, index);
			return index;
		}
	}
	*length = 0;
	LOG("SOUND REMAPPING", "Failed to located sound \"%s\"", path);
	return 0;
}

bool fix11_ImportWav(CHK* v2, CHK* v3, EUDSettings * settings)
{

	/*
		Visor use: eeea.wav
		View abilities statuses/ abilities recharged / : ccca.wav
		Regular shot: Gun.wav
		Gun reload: selectaaepaaep.wav
		Background music: bbbbbbbbb.wav
	
	*/

	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");
	
	// Create new sound collection for original v2S data
	SoundCollection* v2S = v2->getSounds();
	SoundCollection* v3S = v3->getSounds();
	Array<char*> fileNames;
	Array<char*> contents;
	
	Array<unsigned int> contentsLengths;
	for (unsigned int fileIndex = 0; fileIndex < v2S->files.getSize(); fileIndex++) {
		SoundFile* file = v2S->files[fileIndex];
		fileNames.append(file->fileName);
		contents.append(file->contents);
		contentsLengths.append(file->contentsSize);
	}
	bool error = false;
	SoundCollection _S(&contents, &contentsLengths, &fileNames, &error);
	if (error) {
		return false;
	}
	// Empty V2S
	for (unsigned int fileIndex = 0; fileIndex < v2S->files.getSize(); fileIndex++) {
		SoundFile* file = v2S->files[fileIndex];
		v2S->deleteSound(file->fileName);
	}
	// Switch variables
	SoundCollection* S = v2S;
	v2S = &_S;
	

	// First import sounds that are to be used by external map
	Array<Action*> wavActions;
	Array<unsigned short>* wavIndexes = v3->getUsedWavs(&wavActions, &error);
	if (error) {
		delete wavIndexes;
		return false;
	}
	Array<unsigned short> wavRemapping;
	// Remap indexes
	LOG("SOUND REMAPPING", "Begin all sound remap");
	for (unsigned int wavArrayIndex = 0; wavArrayIndex < wavIndexes->getSize(); wavArrayIndex++) {
		unsigned short wavIndex = wavIndexes->get(wavArrayIndex);
		char* wavName = v3STR->getRawString(wavIndex);
		unsigned int length = strlen(wavName);
		MALLOC_N(newWavName, char, length + 4, { delete wavIndexes; return false; });
		memcpy(newWavName + 2, wavName, length);
		newWavName[0] = 'C';
		newWavName[1] = ':';
		newWavName[2 + length] = 0;
		bool error = false;
		unsigned short index = importSound(v2STR, v2S, v3S, S, newWavName, &length, &error);
		if (error) {
			free(newWavName);
			continue;
		}
		wavRemapping.append(index);
		free(newWavName);
	}
	// Remap actions to those new indexes
	for (unsigned int actionIndex = 0; actionIndex < wavActions.getSize(); actionIndex++) {
		Action* wavAction = wavActions[actionIndex];
		unsigned short wavIndex = wavAction->WAVStringNumber;
		char* wavName = v3STR->getRawString(wavIndex);
		bool _continue = false;
		wavAction->WAVStringNumber = 0; // No sound
		for (unsigned int wavArrayIndex = 0; wavArrayIndex < wavIndexes->getSize(); wavArrayIndex++) {
			if (wavIndexes->get(wavArrayIndex) == wavIndex ) { // Found index to remapping array
				unsigned short newIndex = wavRemapping[wavArrayIndex];
				wavAction->WAVStringNumber = newIndex;
				if (newIndex == 0) {
					LOG("SOUND REMAPPING", "Disabling action #%d which used wav at index %d (\"%s\")", actionIndex, wavIndex, wavName);
					wavAction->Flags |= 2;
				}
				else {
					LOG("SOUND REMAPPING", "Remapping action #%d which used wav at index %d (\"%s\") to new index %d", actionIndex, wavIndex, wavName, newIndex);
				}
				_continue = true;
			}
		}
		if (_continue) {
			continue;
		}
		LOG("SOUND REMAPPING", "Disabling action #%d because new index for wav index %d (\"%s\") could not be found. This should never happen", actionIndex, wavIndex, wavName);
		wavAction->Flags |= 2;
	}
	delete wavIndexes;
	LOG("SOUND REMAPPING", "End all sound remap");

	unsigned short bgMusicIndex = 0;
	unsigned short visorMusicIndex = 0;
	unsigned short gunMusicIndex = 0;
	unsigned int length;
	if (settings->useDefaultBackgroundMusic && settings->BackgroundWavFilePath != nullptr) { // Enable background music
		bool error = false;
		bgMusicIndex = importSound(v2STR, v2S, v3S, S, (char*)settings->BackgroundWavFilePath, &length, &error);
		if (error) {
			return false;
		}
		v2TRIG->triggers.get(245)->conditions[0].Quantifier = length * 25; // Adjust background delay
	}
	if (settings->enableVisor && settings->VisorUsageFilePath != nullptr) { // Enable background music
		visorMusicIndex = importSound(v2STR, v2S, v3S, S, (char*)settings->VisorUsageFilePath, &length,&error);
		if (error) {
			return false;
		}
	}
	if (settings->useDefaultGunShot && settings->GunShotWavFilePath != nullptr) { // Enable background music
		gunMusicIndex = importSound(v2STR, v2S, v3S, S, (char*)settings->GunShotWavFilePath, &length, &error);
		if (error) {
			return false;
		}
	}

#define SET_SND_INDEX(TRIGGER_INDEX, ACTION_INDEX, VALUE) {\
	if(VALUE == 0) {\
		v2TRIG->triggers.get(TRIGGER_INDEX)->actions[ACTION_INDEX].Flags |= 2;\
	} else {\
		v2TRIG->triggers.get(TRIGGER_INDEX)->actions[ACTION_INDEX].WAVStringNumber = VALUE;\
	}\
}
	
	SET_SND_INDEX(312, 2, visorMusicIndex);
	SET_SND_INDEX(243, 1, gunMusicIndex);
	SET_SND_INDEX(246, 0, bgMusicIndex);
	
	// Abilities usage sound
	unsigned short ccaSoundIndex = importSound(v2STR, v2S, v3S, S, "S:staredit\\wav\\ccca.wav", &length, &error);
	if (error) {
		return false;
	}
	SET_SND_INDEX(311, 2, ccaSoundIndex);
	SET_SND_INDEX(313, 1, ccaSoundIndex);
	SET_SND_INDEX(314, 1, ccaSoundIndex);
	SET_SND_INDEX(315, 1, ccaSoundIndex);
	SET_SND_INDEX(316, 2, ccaSoundIndex);

	// Gun reload sound
	unsigned short gunReloadSound = importSound(v2STR, v2S, v3S, S, "S:staredit\\wav\\selectaaepaaep.wav", &length, &error);
	if (error) {
		return false;
	}
	SET_SND_INDEX(249, 1, gunReloadSound);
	SET_SND_INDEX(272, 4, gunReloadSound);
	SET_SND_INDEX(274, 4, gunReloadSound);
	SET_SND_INDEX(276, 4, gunReloadSound);
	SET_SND_INDEX(278, 4, gunReloadSound);
	SET_SND_INDEX(280, 4, gunReloadSound);
	SET_SND_INDEX(282, 4, gunReloadSound);
	SET_SND_INDEX(321, 2, gunReloadSound);
	return true;
}

bool fix12_DisableEndGames(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");

	v2TRIG->triggers.get(241)->actions[0].Flags |= 2; // Disable victory text
	v2TRIG->triggers.get(242)->actions[0].Flags |= 2; // Disable display defeat text

	for (unsigned int i = 0; i < v2TRIG->triggers.getSize(); i++) {
		Trigger* trigger = v2TRIG->triggers[i];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if (action->ActionType == 1 || action->ActionType == 2 || action->ActionType == 56) {
				LOG("ENDGAME", "Disabling end game in trigger %d (%s)", i, (action->ActionType == 1 ? "Victory" : action->ActionType == 2 ? "Defeat" : "Draw"))
				action->Flags |= 2;
			}
		}
	}
	return true;
}

static const unsigned short weaponsRemapping[] = { 0x00, 0x02, 0x04, 0x82, 0x07, 0x82, 0x0b, 0x0d, 0x10, 0x82, 0x1a, 0x82, 0x13, 0x06, 0x82, 0x82, 0x03, 0x82, 0x09, 0x05, 0x01, 0x12, 0x82, 0x82, 0x0c, 0x82, 0x1c, 0x15, 0x17, 0x15, 0x82, 0x1b, 0x19, 0x82, 0x82, 0x82, 0x82, 0x23, 0x26, 0x28, 0x2a, 0x2b, 0x82, 0x30, 0x2e, 0x82, 0x82, 0x82, 0x29, 0x82, 0x36, 0x25, 0x82, 0x27, 0x24, 0x31, 0x2f, 0x82, 0x82, 0x82, 0x82, 0x6f, 0x82, 0x82, 0x3e, 0x40, 0x42, 0x82, 0x46, 0x82, 0x49, 0x4d, 0x82, 0x4f, 0x56, 0x55, 0x47, 0x41, 0x43, 0x45, 0x4b, 0x82, 0x82, 0x82, 0x82, 0x52, 0x4e, 0x45, 0x72, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x70, 0x74, 0x82, 0x15, 0x6d, 0x71, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x35, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x50, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x5c, 0x82, 0x5d, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x63, 0x82, 0x82, 0x82, 0x82, 0x82, 0x60, 0x61, 0x62, 0x61, 0x62, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82 };
static const unsigned int def_hp[] = { 0x00640000, 0x000b0000, 0x00010000, 0x00010000, 0x00000000, 0x00010000, 0x00000000, 0x00010000, 0x00010000, 0x00010000, 0x00090000, 0x00010000, 0x00010000, 0x00140000, 0x00640000, 0x00280000, 0x00090000, 0x002c0100, 0x00000000, 0x002c0100, 0x00090000, 0x00f40100, 0x00200300, 0x00900100, 0x01000000, 0x00900100, 0x01000000, 0x00e80300, 0x00520300, 0x00bc0200, 0x00010000, 0x00000000, 0x00090000, 0x01000000, 0x00010000, 0x00190000, 0x00c80000, 0x00010000, 0x00010000, 0x00f40100, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00282300, 0x002c0100, 0x00010000, 0x00d00700, 0x00fa0000, 0x00010000, 0x00090000, 0x00010000, 0x00010000, 0x00e80300, 0x00010000, 0x00c80000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00280000, 0x003c0000, 0x00881300, 0x00010000, 0x00f00000, 0x00e80300, 0x00900100, 0x00c80000, 0x00200300, 0x00010000, 0x00010000, 0x00010000, 0x00580200, 0x00500000, 0x00fa0000, 0x003c0000, 0x003c0000, 0x007d0000, 0x007d0000, 0x003c0000, 0x00090000, 0x003c0000, 0x003c0000, 0x00c80000, 0x00640000, 0x00c80000, 0x00fa0000, 0x00010000, 0x00bc0200, 0x00010000, 0x00090000, 0x00200300, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00090000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x01000000, 0x00010000, 0x01000000, 0x00010000, 0x00010000, 0x00d00700, 0x00010000, 0x00bc0200, 0x00d00700, 0x00102700, 0x00102700, 0x00dc0500, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x01000000, 0x00010000, 0x00881300, 0x00c40900, 0x00010000, 0x00fa0000, 0x00dc0500, 0x00dc0500, 0x01000000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x002c0100, 0x00010000, 0x00010000, 0x01000000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00d00700, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00a08601, 0x00dc0500, 0x00881300, 0x00a08601, 0x00a08601, 0x00a08601, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00a08601, 0x00bc0200, 0x00d00700, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00200300, 0x00c40900, 0x00200300, 0x00320000, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00a08601, 0x00320000, 0x00320000, 0x00320000, 0x00320000, 0x00320000, 0x00200300, 0x00a08601, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300, 0x00200300 };
static const unsigned char def_armor[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x04, 0x04, 0x03, 0x00, 0x03, 0x00, 0x04, 0x04, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x02, 0x03, 0x02, 0x00, 0x03, 0x04, 0x04, 0x02, 0x00, 0x01, 0x01, 0x02, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x02, 0x03, 0x00, 0x04, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const unsigned short def_shield[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x6400, 0x6400, 0x0000, 0x6400, 0x0000, 0x6400, 0x0000, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x6400, 0x0000, 0x6400, 0x6400, 0x6400, 0x0000, 0x6400, 0x6400, 0x6400, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x5000, 0x9001, 0x0000, 0x0100, 0xf000, 0xe803, 0x9001, 0x9001, 0xf401, 0x0100, 0x0000, 0x0100, 0xf401, 0x2c01, 0xfa00, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x0000, 0x6400, 0x6400, 0x0000, 0x3c00, 0x6400, 0x6400, 0x0100, 0x6400, 0x0000, 0x0000, 0x6400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x0000, 0x6400, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6400, 0x0000, 0x6400, 0x6400, 0x0000, 0x6400, 0x6400, 0x6400, 0x6400, 0x0100, 0x0100, 0x0100, 0x0100, 0x2c01, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x6400, 0x0100, 0x0100, 0x0100, 0x0100, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x0000, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400 };
static const unsigned short def_damage[] = { 0x0600, 0x1200, 0xe803, 0x1e00, 0x0200, 0x1e00, 0xfb00, 0x0600, 0x0a00, 0x1800, 0x1400, 0x0800, 0x4600, 0x0500, 0x0000, 0x1400, 0x0100, 0x2800, 0x1000, 0x1900, 0x1900, 0x3200, 0x3200, 0x1e00, 0x1e00, 0x0700, 0x1000, 0x0600, 0x9600, 0x1400, 0x0401, 0x5802, 0x0000, 0x0000, 0xfa00, 0x0f00, 0x0f00, 0x3200, 0x3200, 0x3200, 0x3200, 0x3200, 0x3200, 0x3200, 0x0000, 0x1e00, 0x7f00, 0x7f00, 0x0000, 0x3200, 0x0500, 0x0a00, 0x0f00, 0x2800, 0x50c3, 0x6e00, 0x0000, 0x0000, 0x0000, 0x0000, 0x2c01, 0x0000, 0x0500, 0x0000, 0x0800, 0x3200, 0x1600, 0x2d00, 0x0500, 0x1400, 0x1e00, 0x3c00, 0x0400, 0x0100, 0x0e00, 0x1400, 0x1c00, 0xe803, 0x1400, 0x0200, 0xf501, 0xfb00, 0x6400, 0x0000, 0x0e00, 0x6400, 0x2d00, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0400, 0x1e00, 0x0a00, 0x0a00, 0x0800, 0x0a00, 0x0500, 0x0000, 0x1400, 0x0600, 0x1900, 0x0800, 0x0800, 0x0800, 0x0000, 0x3200, 0x0600, 0x2800, 0x1900, 0x0000, 0x1400, 0x1c00, 0x1e00, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600 };

bool fix0_ResetUnusedUnitsToTheirDefaultValues(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_UNIx, v3UNIx, v3, "UNIx");

	for (unsigned int i = 0; i < 228; i++) {
		if (v3UNIx->data->used[i] == 1) { // Use default values
			unsigned int HP = v3UNIx->data->hp[i]; // Damage intended for <ghostHP>
			unsigned char armor = v3UNIx->data->armor[i]; // Damage intended for <ghostHP>
			unsigned short shield = v3UNIx->data->shield[i];
			v3UNIx->data->hp[i] = def_shield[i];
			v3UNIx->data->armor[i] = def_armor[i];
			v3UNIx->data->shield[i] = def_shield[i];
			v3UNIx->data->hp[i] = def_shield[i];
			unsigned short weaponID = weaponsRemapping[i];
			if (weaponID < 130) { // Some weapon is there
				v3UNIx->data->weapon_damage[weaponID] = def_damage[weaponID];
			}
		}
	}
	return true;
}

bool fix13_RecalculateHPAndDamage(CHK* v2, CHK* v3, EUDSettings* settings) {
	if (!fix0_ResetUnusedUnitsToTheirDefaultValues(v2, v3, settings)) {
		return false;
	}
	GET_SECT(Section_UNIx, v3UNIx, v3, "UNIx");

	unsigned int ghostHP = v3UNIx->data->hp[1];
	unsigned int ghostArmor = v3UNIx->data->armor[1];

	double factor = ((double) 11.0 * 256) / ((double)ghostHP);
	for (unsigned int i = 0; i < 130; i++) {
		unsigned int damage = v3UNIx->data->weapon_damage[i]; // Damage intended for <ghostHP>
		unsigned int newDamage = (unsigned int) ceil(damage * factor);
		LOG("DAMAGE REMAPPER", "Changing damage of weapon id %d from %d to %d", i, damage, newDamage);
		v3UNIx->data->weapon_damage[i] = newDamage;
	}

	// Calculate which units are really present in map and which can be created by triggers
	unsigned char used[228];
	memset(used, 0, 228);

	// Can be created by trigger
	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");
	for (unsigned int i = 0; i < v3TRIG->triggers.getSize(); i++) {
		Trigger* trigger = v3TRIG->triggers[i];
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if ((action->ActionType == 44 || action->ActionType == 11) && (action->Flags & 2) == 0) { // Creates unit & Create unit with properties
				unsigned short unitID = action->UnitType;
				if (unitID >= 228) {
					LOG("UNIT PROPERTIES REMAPPER", "Found unit with ID %d", unitID);
				}
				else {
					used[unitID] = 1;
				}
			}
		}
	}
	// Exists in map
	GET_SECT(Section_UNIT, v3UNIT, v3, "UNIT");
	for (unsigned int unitIndex = 0; unitIndex < v3UNIT->units.getSize(); unitIndex++) {
		Unit* unit = v3UNIT->units.get(unitIndex);
		unsigned short unitID = unit->unitID;
		if (unitID >= 228) {
			LOG("UNIT PROPERTIES REMAPPER", "Found unit with ID %d", unitID);
		}
		else {
			used[unitID] = 1;
		}
	}
#define COEXISTS(id1, id2) used[id2] = used[id1];
	// Fix turrets
	COEXISTS(3, 4);
	COEXISTS(5, 6);
	COEXISTS(17, 18);
	COEXISTS(23, 24);
	COEXISTS(25, 26);
	COEXISTS(30, 31);

	UnitSettings* prefferedSettings = (UnitSettings*)settings->preferredUnitSettings;
	

	for (unsigned int i = 0; i < 228; i++) {
		unsigned int HP = v3UNIx->data->hp[i]; // Damage intended for <ghostHP>
		unsigned int newHP = (unsigned int)ceil(HP * factor);
		unsigned int armor = v3UNIx->data->armor[i]; // Damage intended for <ghostHP>
		unsigned int newArmor = (unsigned int)ceil(armor * factor);
		unsigned int shield = v3UNIx->data->shield[i];
		unsigned int newShield = (unsigned int)ceil(shield * factor);
		if (newHP == 0) {
			newHP = 1;
		}
		if(prefferedSettings != nullptr) { // Not a precalculation
			if (prefferedSettings->used[i] == 1) { // Override using this data
				newHP = prefferedSettings->hp[i];
				newArmor = prefferedSettings->armor[i];
				newShield = prefferedSettings->shield[i];
				unsigned short weaponID = weaponsRemapping[i];
				if (weaponID < 130) { // Some weapon is there
					v3UNIx->data->weapon_damage[weaponID] = prefferedSettings->weapon_damage[weaponID];
				}
			}
		}

		LOG("HP REMAPPER", "Changing HP of unit id %d from %d to %d, armor from %d to %d and shields from %d to %d", i, HP/256, newHP/256, armor, newArmor, shield, newShield);
		v3UNIx->data->hp[i] = newHP;
		v3UNIx->data->armor[i] = newArmor;
		v3UNIx->data->shield[i] = newShield;
		v3UNIx->data->used[i] = !used[i]; // Enable usage of everything so that it takes effect
	}


	if (v2 != nullptr) {
		GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
		unsigned int originalEMP = settings->EMPDamage;
		settings->EMPDamage = (unsigned int)ceil(settings->EMPDamage * factor);
		LOG("DAMAGE REMAPPER", "Changing damage of EMP from %d to %d", originalEMP, settings->EMPDamage);
		v2TRIG->triggers.get(235)->actions[25].Group = settings->EMPDamage;

		// Adjust ignore armor flags
		bool remapArmor[130];
		for (unsigned int i = 0; i < 130; i++) {
			remapArmor[i] = (((char*)settings->weaponsToIgnoreArmors)[i] == 4)  ? true : false;
		}
		remapArmor[56] = true; // EMP

		unsigned char defaultRemappings[130] = { 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x01, 0x03, 0x01, 0x03, 0x01, 0x01, 0x01, 0x03, 0x03, 0x01, 0x03, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x04, 0x03, 0x03, 0x03, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x03, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x03, 0x01, 0x03, 0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x00, 0x04, 0x03, 0x03, 0x01, 0x03, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x02, 0x01, 0x01, 0x04, 0x04, 0x01, 0x01, 0x03, 0x04, 0x00, 0x00, 0x03, 0x03, 0x03, 0x02, 0x02, 0x03, 0x01, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 };

		MALLOC_N(remapTrigger, Trigger, 1, { return false; });
		memset(remapTrigger, 0, sizeof(Trigger));
		remapTrigger->players[17] = 1;  // All players
		remapTrigger->conditions[0].ConditionType = 22; // Always

		// It can be done by a single trigger
		unsigned int baseOffset = 209853;
		for (unsigned int offset = 0; offset < 130; offset+=4) {
			unsigned int actionIndex = offset / 4;
			Action* act = &(remapTrigger->actions[actionIndex]);
			unsigned char v1 = remapArmor[offset + 0] ? 4 : defaultRemappings[offset + 0];
			unsigned char v2 = remapArmor[offset + 1] ? 4 : defaultRemappings[offset + 1];
			unsigned char v3 = remapArmor[offset + 2] ? 4 : defaultRemappings[offset + 2];
			unsigned char v4 = remapArmor[offset + 3] ? 4 : defaultRemappings[offset + 3];
			unsigned int value = (v4 << 24) | (v3 << 16) | (v2 << 8) | (v1 << 0);
			act->ActionType = 45;  // Set deaths
			act->Player = baseOffset + actionIndex;  // Offset of the item
			act->UnitType = 0;  // Set deaths of marine
			act->UnitsNumber = 7;  // Set to
			act->Group = value;  // Set value
		}
		v2TRIG->triggers.append(remapTrigger);
	}
	return true;
}

bool fix14_CopySections(CHK* v2, CHK* v3, EUDSettings* settings) {
	char* sections[] = { "ERA ", "MTXM", "TILE", "THG2", "MASK", "UNIT", "DIM ", "UPGS", "UPGx", "TECx", "TECS", "FORC", "OWNR", "COLR", "PTEx", "PUPx" };
	unsigned int sectionsLength = sizeof(sections) / sizeof(char*);
	for (unsigned int sectionIndex = 0; sectionIndex < sectionsLength; sectionIndex++) {
		char* sectionName = sections[sectionIndex];
		Section* newSection = v3->getSection(sectionName);
		if (newSection == nullptr) {
			LOG("SECTIONS", "Section %s not found", sectionName);
		}
		else {
			v3->removeSection(newSection);
			v2->setSection(newSection);
		}
	}
	return true;
}

bool fix15_CopyScenarionNameAndDescription(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	GET_SECT(Section_SPRP, v2SPRP, v2, "SPRP");
	GET_SECT(Section_SPRP, v3SPRP, v3, "SPRP");


	char* scenarioName = v3STR->getRawString(v3SPRP->str_scenarioName);
	char* scenarioDescription = v3STR->getRawString(v3SPRP->str_scenarioDescription);
	v2SPRP->str_scenarioName = v2STR->getNewStringIndex(scenarioName);
	v2SPRP->str_scenarioDescription = v2STR->getNewStringIndex(scenarioDescription);
	LOG("SCENARIO", "Set scenario name to %s", v2STR->getRawString(v2SPRP->str_scenarioName));
	LOG("SCENARIO", "Set scenario description to %s", v2STR->getRawString(v2SPRP->str_scenarioDescription));
	return true;
}

bool fix16_CopyTriggersAndBriefing(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	GET_SECT(Section_TRIG, v3TRIG, v3, "TRIG");

	GET_SECT(Section_MBRF, v2MBRF, v2, "MBRF");
	GET_SECT(Section_MBRF, v3MBRF, v3, "MBRF");

	for (unsigned int type = 0; type < 2; type++) {
		Section_TRIG* SRC = type == 0 ? v3TRIG : v3MBRF;
		Section_TRIG* DST = type == 0 ? v2TRIG : v2MBRF;

		for (unsigned int triggerIndex = 0; triggerIndex < SRC->triggers.getSize(); triggerIndex++) {
			Trigger* srcTrigger = SRC->triggers[triggerIndex];
			MALLOC_N(trigger, Trigger, 1, { return false; });
			memcpy(trigger, srcTrigger, sizeof(Trigger));

			for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
				Action* action = &(trigger->actions[actionIndex]);
				if (action->ActionType == 7 || action->ActionType == 9 || action->ActionType == 12 || action->ActionType == 17 || action->ActionType == 18 || action->ActionType == 19 || action->ActionType == 20 || action->ActionType == 21 || action->ActionType == 33 || action->ActionType == 34 || action->ActionType == 35 || action->ActionType == 36 || action->ActionType == 37) {
					if (action->TriggerText != 0) {
						char* triggerText = v3STR->getRawString(action->TriggerText);
						if (strlen(triggerText) == 0) {
							action->TriggerText = 0;
						} else{
							action->TriggerText = v2STR->getNewStringIndex(triggerText); 
						}
					}
				}
			}
			if (!DST->triggers.append(trigger)) {
				return false;
			}
		}
	}
	return true;
}

bool fix17_CopyUnitSettings(CHK* v2, CHK* v3, EUDSettings* settings) {
	GET_SECT(Section_UNIx, v2UNIX, v2, "UNIx");
	GET_SECT(Section_UNIx, v3UNIX, v3, "UNIx");

	GET_SECT(Section_STR_, v2STR, v2, "STR ");
	GET_SECT(Section_STR_, v3STR, v3, "STR ");

	memcpy(v2UNIX->data, v3UNIX->data, sizeof(UnitSettings));

	for (unsigned int unitIndex = 0; unitIndex < 228; unitIndex++) {
		unsigned short unitNameIndex = v3UNIX->data->str_unit_name[unitIndex];
		if (unitNameIndex != 0) {
			char* unitName = v3STR->getRawString(unitNameIndex);
			v2UNIX->data->str_unit_name[unitIndex] = v2STR->getNewStringIndex(unitName);
			LOG("UNITS", "Copying \"%s\" as units %d name", unitName, unitIndex);
		}
	}
	return true;
}

bool fix18_RelocateSTREUDSection(CHK * v2, CHK * v3, EUDSettings * settings) {

	/* Experiments 
	Section_STR_* STR = (Section_STR_*)v2->getSection("STR ");

	WriteBuffer wb;
	v2->write(&wb);

	
	
	unsigned char* data;
	unsigned int rawDataLength;
	wb.getWrittenData(&data, &rawDataLength);

	rawDataLength -= 2400;

	unsigned int lookUp = 11;

	for (unsigned int index = 0; index < rawDataLength; index++) {
		unsigned char byte = data[index];
		Action* action = (Action*)(&(data[index]));
		if (action->ActionType == 45 && action->UnitsNumber == 7) {
			if ((action->Group >> 24) & 0xff == lookUp || (action->Group >> 16) & 0xff == lookUp || (action->Group >> 8) & 0xff == lookUp || (action->Group >> 0) & 0xff == lookUp) {
				unsigned char unit = STR->data[index + 7];
				LOG("STR ANALYZER", "Found 12 at %d for PID %d", index, action->Player);
			}
		}
	}

	/*
	
	STR->data[176244] = 51; // Barrier restore HP
	v2TRIG->triggers.get(313)->actions[3].Group = 51;
	v2TRIG->triggers.get(314)->actions[3].Group = 51;
	v2TRIG->triggers.get(315)->actions[3].Group = 51;
	v2TRIG->triggers.get(316)->actions[1].Group = 51;
	
	STR->data[58752] = 51;
	STR->data[72932] = 51;
	//STR->data[141308] = 51;
	STR->data[157092] = 51;
	STR->data[176244] = 51;

	STR->data[203948] = 51;
	STR->data[306528] = 51;
	STR->data[374860] = 51;
	STR->data[374956] = 51;
	STR->data[391060] = 51;

	STR->data[482276] = 51;
	*/
	return true;

#ifdef TRIG_PRINT
	/*
	GET_SECT(Section_TRIG, v2TRIG, v2, "TRIG");
	unsigned int shiftOffset = 102400;
	{
		MALLOC_N(trigger, Trigger, 1);
		Trigger* templ = v2TRIG->triggers[193];
		memset(trigger, 0, sizeof(Trigger));
		trigger->flags = 4;
		trigger->conditions[0].ConditionType = 22;
		trigger->conditions[0].Flags = 4;

		trigger->actions[0].ActionType = templ->actions[1].ActionType;
		trigger->actions[0].Player = templ->actions[1].Player;
		trigger->actions[0].Group = shiftOffset;
		trigger->actions[0].UnitsNumber = templ->actions[1].UnitsNumber;
		trigger->actions[0].Flags = templ->actions[1].Flags;

		trigger->actions[1].ActionType = templ->actions[2].ActionType;
		trigger->actions[1].Player = templ->actions[2].Player;
		trigger->actions[1].Group = shiftOffset;
		trigger->actions[1].UnitsNumber = templ->actions[2].UnitsNumber;
		trigger->actions[1].Flags = templ->actions[2].Flags;


		v2TRIG->triggers.insert(194, trigger);
	}

	{
		MALLOC_N(trigger, Trigger, 1);
		Trigger* templ = v2TRIG->triggers[224];
		memset(trigger, 0, sizeof(Trigger));
		trigger->flags = 4;
		trigger->conditions[0].ConditionType = 22;
		trigger->conditions[0].Flags = 4;

		trigger->actions[0].ActionType = templ->actions[1].ActionType;
		trigger->actions[0].Player = templ->actions[1].Player;
		trigger->actions[0].Group = shiftOffset;
		trigger->actions[0].UnitsNumber = templ->actions[1].UnitsNumber;
		trigger->actions[0].Flags = templ->actions[1].Flags;

		v2TRIG->triggers.insert(225, trigger);
	}
	*/
	//v2TRIG->print(190, 10);
	//v2TRIG->print(220, 10);
#endif
}

