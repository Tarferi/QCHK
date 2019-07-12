#pragma once

#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "common.h"

class Section
{
public:
	Section(unsigned char* name, unsigned int size, ReadBuffer* buffer);
	virtual ~Section();
	bool process();

	char* getName() {
		return this->name;
	}

	virtual bool write(WriteBuffer* buffer) = 0;
	unsigned int bufferBeginPosition;
	unsigned int size;

protected:
	ReadBuffer* buffer;
	char* name;
	
	virtual bool parse() = 0;


private:
	bool processed = false;
	
};

class BasicSection : public Section {

public:

	COMMON_CONSTR_SEC(BasicSection)

	virtual ~BasicSection() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

protected:
	bool parse() {
		bool error = false;
		this->data = this->buffer->readArray(this->size, &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray(this->data, this->size, &error);
		return !error;
	}

private:
	unsigned char* data = nullptr;

};

class Section_TYPE : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_TYPE)

};

class Section_VER_ : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_VER_)

};

class Section_IVER : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_IVER)

};

class Section_IVE2 : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_IVE2)

};

class Section_VCOD : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_VCOD)

};

class Section_STR_ : public Section {

	struct Section_STR_Offset {
		unsigned short offset;
		unsigned short index;
	};

public:

	unsigned int getNewStringIndex(char* string) {
		unsigned int off = 2 + (2 * this->totalStrings); // Offset list
		unsigned int freePosition = this->lastUsedOffset;
		unsigned int stringLength = strlen(string);
		this->offsets[this->lastUsedOffsetIndex] = freePosition + off;
		memcpy(&(data[freePosition]), string, stringLength + 1);
		this->lastUsedOffsetIndex++;
		this->lastUsedOffset += stringLength + 1; // After \0 delimiter
		LOG("STR", "Allocating new index %d for string \"%s\"", this->lastUsedOffsetIndex, string);
		return this->lastUsedOffsetIndex;
	}

	char* getRawString(unsigned int index) {
		unsigned int off = 2 + (2 * this->totalStrings);
		unsigned short offset = this->offsets[index - 1];
		return (char*) (&(this->data[offset - off]));
	}

	void setRawString(unsigned int index, char* string) {
		unsigned int off = 2 + (2 * this->offsets.getSize());
		unsigned int offset = this->offsets[index - 1] - off;
		unsigned int len = strlen(string);
		for (unsigned int i = 0; i <= len; i++) {
			this->data[offset + 1 + i] = string[i];
		}
	}

	COMMON_CONSTR_SEC(Section_STR_)

	virtual ~Section_STR_() {
		if (this->sortedOffsets != nullptr) {
			delete[] this->sortedOffsets;
			this->sortedOffsets = nullptr;
		}
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

	unsigned char* data = nullptr;
	unsigned short totalStrings;

protected:
	bool parse() {
		this->free_space_begin = 0x0CF8;
		this->free_space_end = 0x19CF;
		this->lastUsedOffset = 0;
		this->lastUsedOffsetIndex = 0;
		this->lastUsedIndex = 0;
		this->sortedOffsets = nullptr;

		bool error = false;
		this->totalStrings = this->buffer->readShort(&error);
		if (error) {
			return false;
		}
		
		for (unsigned int i = 0; i < this->totalStrings; i++) {
			unsigned short offset = this->buffer->readShort(&error);
			if (error) {
				return false;
			}
			offsets.append(offset);
		}
		unsigned int totalStrings = this->offsets.getSize();
		this->sortedOffsets = new Section_STR_Offset[totalStrings];

		for (unsigned int i = 0; i < totalStrings; i++) {
			unsigned short offset = this->offsets[i];
			Section_STR_Offset* o = &(this->sortedOffsets[i]);

			o->index = i;
			o->offset = offset;
		}
		bool changed = false;
		do {
			changed = false;
			for (unsigned int i = 1; i < this->offsets.getSize(); i++) {
				Section_STR_Offset* off0 = &(this->sortedOffsets[i - 1]);
				Section_STR_Offset* off1 = &(this->sortedOffsets[i]);
				if (off0->offset > off1->offset) {
					unsigned short tmp_offset = off0->offset;
					unsigned int tmp_index = off0->index;
					off0->index = off1->index;
					off0->offset = off1->offset;
					off1->index = tmp_index;
					off1->offset = tmp_offset;
				}
			}
		} while (changed);
		this->data = this->buffer->readArray(this->size - (2 + (2 * this->totalStrings)), &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeShort(this->totalStrings,&error);
		if (error) {
			return false;
		}
		for (unsigned int i = 0; i<totalStrings; i++) {
			buffer->writeShort(this->offsets[i], &error);
			if (error) {
				return false;
			}
		};
		unsigned int off = 2 + (2 * this->totalStrings);
		unsigned int rawDataLength = this->size - (2 + (2 * this->totalStrings));
		buffer->writeArray(this->data, rawDataLength, &error);
		return !error;
	}

private:

	Array<unsigned short> offsets;
	Section_STR_Offset* sortedOffsets = nullptr;



	unsigned int free_space_begin;
	unsigned int free_space_end;
	unsigned int lastUsedIndex;
	unsigned int lastUsedOffset;
	unsigned int lastUsedOffsetIndex;
};

class Section_IOWN : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_IOWN)

};

struct Section_OWNR_STRUCT {
	unsigned char slots[12];
};

class Section_OWNR : public Section {

public:

	COMMON_CONSTR_SEC(Section_OWNR)

	Section_OWNR_STRUCT* data = nullptr;

	virtual ~Section_OWNR() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

protected:
	bool parse() {
		bool error = false;
		this->data = (Section_OWNR_STRUCT*) this->buffer->readArray(sizeof(Section_OWNR_STRUCT), &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray((unsigned char*) this->data, sizeof(Section_OWNR_STRUCT), &error);
		return !error;
	}
};

class Section_ERA_ : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_ERA_)

};

class Section_DIM_ : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_DIM_)

};

class Section_SIDE : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_SIDE)

};

class Section_MTXM : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_MTXM)

};

class Section_PUNI : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_PUNI)

};

class Section_UPGR : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UPGR)

};

class Section_PTEC : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_PTEC)

};

class Section_UNIT : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UNIT)

};

class Section_ISOM : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_ISOM)

};

class Section_TILE : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_TILE)

};

class Section_DD2_ : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_DD2_)

};

class Section_THG2 : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_THG2)

};

class Section_MASK : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_MASK)

};

class Section_UPRP : public Section {

public:

	COMMON_CONSTR_SEC(Section_UPRP)

	virtual ~Section_UPRP() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

	bool parse() {
		this->dataSize = this->size;
		bool error = false;
		this->data = this->buffer->readArray(this->dataSize, &error);
		return !error;
	}

	bool write(WriteBuffer* wb) {
		bool error = false;
		wb->writeArray(this->data, this->dataSize, &error);
		return !error;
	}

	unsigned char* data = nullptr;
	unsigned int dataSize;

};

class Section_UPUS : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UPUS)

};

struct Location {
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
	unsigned short str_description;
	unsigned short elevation;
};

class Section_MRGN : public Section {

public:

	Array<Location*> locations;

	COMMON_CONSTR_SEC(Section_MRGN)

	virtual ~Section_MRGN() {
		for (unsigned int i = 0; i < locations.getSize(); i++) {
			if (locations[i] != nullptr) {
				Location* loc = locations[i];
				free(loc);
				locations[i] = nullptr;
			}
		}
	}

protected:

	bool parse() {

		unsigned int totalLocations = this->size / sizeof(Location);
		bool error = false;
		for (unsigned int i = 0; i < totalLocations; i++) {
			Location* location = (Location*) this->buffer->readArray(sizeof(Location), &error);
			if (error) {
				return false;
			}
			this->locations.append(location);
		}
		return true;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		for (unsigned int i = 0; i < this->locations.getSize(); i++) {
			Location* locations = this->locations[i];
			buffer->writeArray((unsigned char*)locations, sizeof(Location), &error);
			if (error) {
				return false;
			}
		};
		return true;
	}


};

typedef struct Action {

	unsigned int SourceLocation;
	unsigned int TriggerText;
	unsigned int WAVStringNumber;
	unsigned int Time;
	unsigned int Player;
	unsigned int Group;
	unsigned short UnitType;
	unsigned char ActionType;
	unsigned char UnitsNumber;
	unsigned char Flags;
	unsigned char Unused[3];

} Action;

typedef struct Condition {
	unsigned int locationNumber;
	unsigned int groupNumber;
	unsigned int Quantifier;
	unsigned short UnitID;
	unsigned char Comparision;
	unsigned char ConditionType;
	unsigned char Resource;
	unsigned char Flags;
	unsigned short Unused;

} Condition;

typedef struct Trigger {
	
	Condition conditions[16];
	Action actions[64];

	unsigned int flags;
	unsigned char players[28];

} Trigger;

#ifdef TRIG_PRINT
#ifdef _DEBUG
class FIELDTYPE;


class TriggerContents {

public:
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error);

	virtual ~TriggerContents();

protected:
	FIELDTYPE* data1 = nullptr;
	FIELDTYPE* data2 = nullptr;
	FIELDTYPE* data3 = nullptr;
	FIELDTYPE* data4 = nullptr;
	FIELDTYPE* data5 = nullptr;
	FIELDTYPE* data6 = nullptr;
	FIELDTYPE* data7 = nullptr;
	FIELDTYPE* data8 = nullptr;
	FIELDTYPE* data9 = nullptr;
	unsigned int dataCount;
	const char* name;
	const char* templateStr;
	const char* invalidTemplateStr;
};
#endif
#endif

class Section_TRIG : public Section {

public:

	COMMON_CONSTR_SEC(Section_TRIG)

		virtual ~Section_TRIG();

	Array<Trigger*> triggers;

#ifdef TRIG_PRINT
#ifdef _DEBUG
	bool print(unsigned int from, unsigned int length, Section_STR_* STR, WriteBuffer* wb) {
		unsigned int limit = from + length > this->triggers.getSize() ? this->triggers.getSize() : from + length;
		bool error = false;
		for (unsigned int i = from; i < limit; i++) {
			this->printTrigger(i, STR, wb, &error);
			if (error) {
				return false;
			}
		}
		return true;
	}

	bool print(Section_STR_* STR, WriteBuffer* wb) {
		return print(0, this->triggers.getSize(), STR, wb);
	}
#endif
#endif

protected:

	bool parse() {

		unsigned int totalTriggers = this->size / sizeof(Trigger);
		bool error = false;
		for (unsigned int i = 0; i < totalTriggers; i++) {
			Trigger* trigger = (Trigger*) this->buffer->readArray(sizeof(Trigger), &error);
			if (error) {
				return false;
			}
			this->triggers.append(trigger);
		}
		return true;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		for (unsigned int i = 0; i < this->triggers.getSize(); i++) {
			Trigger* trigger = this->triggers[i];
			buffer->writeArray((unsigned char*)trigger, sizeof(Trigger), &error);
			if (error) {
				return false;
			}
		};
		return true;
	}

#ifdef TRIG_PRINT
#ifdef _DEBUG

#define PRINT_R(ignore, format, ...) {char buffer[1024]; sprintf_s(buffer, format, __VA_ARGS__); wb->writeFixedLengthString((unsigned char*) buffer, error); if(*error) {return;}}

	void printAction(Action* action, Section_STR_* STR, WriteBuffer* wb, bool* error);

	void printCondition(Condition* condition, Section_STR_* STR, WriteBuffer* wb, bool* error);

	void printPlayers(Trigger* trigger, Section_STR_* STR, WriteBuffer* wb, bool* error);
	
	void printTrigger(unsigned int triggerIndex, Section_STR_* STR, WriteBuffer* wb, bool* error) {
		Trigger* trigger = this->triggers[triggerIndex];

		
		PRINT_R("TRIGGERS", "Trigger[%d] {Flags: %d} (", triggerIndex, trigger->flags);
		this->printPlayers(trigger, STR, wb, error);
		if (*error) {
			return; 
		}
		PRINT_R("TRIGGERS", ")\r\n");
		PRINT_R("TRIGGERS", "\tConditions:\r\n");
		for (unsigned int conditionIndex = 0; conditionIndex < 16; conditionIndex++) {
			Condition* condition = &(trigger->conditions[conditionIndex]);
			if (condition->ConditionType != 0) {
				PRINT_R("TRIGGERS", "\t\t%s", (condition->Flags & 2) > 0 ? "(disabled)" : "");
				printCondition(condition, STR, wb, error);
				if (*error) {
					return;
				}
				PRINT_R("TRIGGERS", "\r\n");
			}
		}
		PRINT_R("TRIGGERS", "\r\n");
		PRINT_R("TRIGGERS", "\tActions:\r\n");
		for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
			Action* action = &(trigger->actions[actionIndex]);
			if (action->ActionType != 0) {
				PRINT_R("TRIGGERS", "\t\t%s", (action->Flags & 2) > 0 ? "(disabled)" : "");
				printAction(action, STR, wb, error);
				if (*error) {
					return;
				}
				PRINT_R("TRIGGERS", "\r\n");
			}
		}
		PRINT_R("TRIGGERS", "\r\n");
		PRINT_R("TRIGGERS", "\r\n");
	}

#endif
#endif

};

class Section_MBRF : public Section_TRIG {
public:
	Section_MBRF(unsigned char* name, unsigned int size, ReadBuffer* buffer) : Section_TRIG(name, size, buffer) { }

};

class Section_SPRP : public Section {

public:

	COMMON_CONSTR_SEC(Section_SPRP)

	bool parse() {
		bool error = false;
		str_scenarioName = this->buffer->readShort(&error);
		if (error) {
			return false;
		}
		str_scenarioDescription = this->buffer->readShort(&error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeShort(this->str_scenarioName, &error);
		if (error) {
			return false;
		}
		buffer->writeShort(this->str_scenarioDescription, &error);
		return !error;
	}

	unsigned short str_scenarioName;
	unsigned short str_scenarioDescription;


};

class Section_FORC : public Section {

	struct Section_FORC_STRUCT {
		unsigned char active[8];
		unsigned short str_names[4];
		unsigned char slots[4];
	};

public:

	COMMON_CONSTR_SEC(Section_FORC)

	Section_FORC_STRUCT* data = nullptr;

	virtual ~Section_FORC() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

protected:
	bool parse() {
		bool error = false;
		this->data = (Section_FORC_STRUCT*) this->buffer->readArray(sizeof(Section_FORC_STRUCT), &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray((unsigned char*) this->data, sizeof(Section_FORC_STRUCT), &error);
		return !error;
	}

};

class Section_WAV_ : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_WAV_)

};

class Section_UNIS : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UNIS)

};

class Section_UPGS : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UPGS)

};

class Section_TECS : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_TECS)

};

class Section_SWNM : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_SWNM)

};

class Section_COLR : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_COLR)

	virtual ~Section_COLR() {
		if (this->playerColors != nullptr) {
			free(this->playerColors);
			this->playerColors = nullptr;
		}
	}

	unsigned char* playerColors = nullptr;

protected:
	bool parse() {
		bool error = false;
		this->playerColors = this->buffer->readArray(8, &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray(this->playerColors, 8, &error);
		return !error;
	}
	
};

class Section_PUPx : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_PUPx)

};

class Section_PTEx : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_PTEx)

};

struct UnitSettings {
	unsigned char used[228];
	unsigned int hp[228];
	unsigned short shield[228];
	unsigned char armor[228];
	unsigned short build_time[228];
	unsigned short mineral_cost[228];
	unsigned short gas_cost[228];
	unsigned short str_unit_name[228];
	unsigned short weapon_damage[130];
	unsigned short upgrade_bonus[130];
};

class Section_UNIx : public Section {

public:

	UnitSettings* data = nullptr;

	COMMON_CONSTR_SEC(Section_UNIx)

		virtual ~Section_UNIx() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

protected:

	bool parse() {
		bool error = false;
		data = (UnitSettings*) buffer->readArray(sizeof(UnitSettings),&error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray((unsigned char*)data, sizeof(UnitSettings), &error);
		return !error;
	}

};

class Section_UPGx : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_UPGx)

};

class Section_TECx : public BasicSection {

public:

	COMMON_CONSTR_SEC_BS(Section_TECx)

};

#ifdef TRIG_PRINT
#ifdef _DEBUG

class FIELDTYPE {
public:
	FIELDTYPE(unsigned int value) {
		this->value = value;
	}
	unsigned int value;

	virtual void print(Section_STR_* STR, WriteBuffer* wb, bool* error) = 0;

	void printArray(WriteBuffer* wb, bool* error, unsigned int valuesSize, char** values) {
		if (this->value < valuesSize) {
			PRINT_R("TRIGGERS", "%s", values[this->value]);
		}
		else {
			PRINT_R("TRIGGERS", "Invalid value: %d", this->value);
		}
	}

	void printAssoc(WriteBuffer* wb, bool* error, unsigned int valuesSize, char** values) {
		for (unsigned int i = 0; i < valuesSize; i++) {
			char* index = values[i * 2];
			if ((int)index == this->value) {
				char* value = values[(i * 2) + 1];
				PRINT_R("TRIGGERS", "%s", value);
				return;
			}
		}
		PRINT_R("TRIGGERS", "Invalid value: %d", this->value);
	}

};

class FIELDTYPE_NUMBER : public FIELDTYPE {
public:

	FIELDTYPE_NUMBER(unsigned int value) : FIELDTYPE(value) {}

	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		PRINT_R("TRIGGERS", "%u", this->value);
	}
};

class FIELDTYPE_ALLYSTATUS : public FIELDTYPE {
public:

	FIELDTYPE_ALLYSTATUS(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { "Enemy", "Ally", "Allied Victory" };
		this->printArray(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_COMPARISON : public FIELDTYPE {
public:

	FIELDTYPE_COMPARISON(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)0, "at least", (char*)1, "at most", (char*)10, "exactly" };
		this->printAssoc(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_MODIFIER : public FIELDTYPE {
public:

	FIELDTYPE_MODIFIER(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)7, "set to", (char*)8, "add", (char*)9, "subtract" };
		this->printAssoc(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_ORDER : public FIELDTYPE {
public:

	FIELDTYPE_ORDER(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { "Move", "Patrol", "Attack" };
		this->printArray(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_PLAYER : public FIELDTYPE {
public:

	FIELDTYPE_PLAYER(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)0, "Player 1", (char*)1, "Player 2", (char*)2, "Player 3", (char*)3, "Player 4", (char*)4, "Player 5", (char*)5, "Player 6", (char*)6, "Player 7", (char*)7, "Player 8", (char*)8, "Player 9", (char*)9, "Player 10", (char*)10, "Player 11", (char*)11, "Player 12", (char*)13, "CurrentPlayer", (char*)14, "Foes", (char*)15, "Allies", (char*)16, "NeutralPlayers", (char*)17, "AllPlayers", (char*)18, "Force1", (char*)19, "Force2", (char*)20, "Force3", (char*)21, "Force4", (char*)26, "NonAlliedVictoryPlayers" };
		this->printAssoc(wb, error, 22, (char**)values);
	}
};

class FIELDTYPE_PROPSTATE : public FIELDTYPE {
public:

	FIELDTYPE_PROPSTATE(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)4, "Enable", (char*)5, "Disable", (char*)6, "Toggle" };
		this->printAssoc(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_RESOURCE : public FIELDTYPE {
public:

	FIELDTYPE_RESOURCE(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { "Ore", "Gas", "Ore and Gas" };
		this->printArray(wb, error, 3, (char**)values);
	}
};

class FIELDTYPE_SCORE : public FIELDTYPE {
public:

	FIELDTYPE_SCORE(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { "Total", "Units", "Buildings", "UnitsAndBuildings", "Kills", "Razings", "KillsAndRazings", "Custom" };
		this->printArray(wb, error, 8, (char**)values);
	}
};

class FIELDTYPE_SWITCHACTION : public FIELDTYPE {
public:

	FIELDTYPE_SWITCHACTION(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)4, "Set", (char*)5, "Clear", (char*)6, "Toggle", (char*)11, "Random" };
		this->printAssoc(wb, error, 4, (char**)values);
	}
};

class FIELDTYPE_SWITCHSTATE : public FIELDTYPE {
public:

	FIELDTYPE_SWITCHSTATE(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { (char*)2, "Set", (char*)3, "Cleared" };
		this->printAssoc(wb, error, 2, (char**)values);
	}
};

class FIELDTYPE_AISCRIPT : public FIELDTYPE {
public:

	FIELDTYPE_AISCRIPT(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		PRINT_R("TRIGGERS", "%d", this->value);
	}
};

class FIELDTYPE_COUNT : public FIELDTYPE {
public:

	FIELDTYPE_COUNT(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		if (this->value == 0) {
			PRINT_R("TRIGGERS", "All");
		}
		else {
			PRINT_R("TRIGGERS", "%d", this->value);
		}
	}
};

class FIELDTYPE_UNIT : public FIELDTYPE {
public:

	FIELDTYPE_UNIT(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		char* values[] = { "Terran Marine", "Terran Ghost", "Terran Vulture", "Terran Goliath", "Goliath Turret", "Terran Siege Tank (Tank Mode)", "Tank Turret(Tank Mode)", "Terran SCV", "Terran Wraith", "Terran Science Vessel", "Gui Montang (Firebat)", "Terran Dropship", "Terran Battlecruiser", "Vulture Spider Mine", "Nuclear Missile", "Terran Civilian", "Sarah Kerrigan (Ghost)", "Alan Schezar (Goliath)", "Alan Schezar Turret", "Jim Raynor (Vulture)", "Jim Raynor (Marine)", "Tom Kazansky (Wraith)", "Magellan (Science Vessel)", "Edmund Duke (Siege Tank)", "Edmund Duke Turret", "Edmund Duke (Siege Mode)", "Edmund Duke Turret", "Arcturus Mengsk (Battlecruiser)", "Hyperion (Battlecruiser)", "Norad II (Battlecruiser)", "Terran Siege Tank (Siege Mode)", "Tank Turret (Siege Mode)", "Firebat", "Scanner Sweep", "Terran Medic", "Zerg Larva", "Zerg Egg", "Zerg Zergling", "Zerg Hydralisk", "Zerg Ultralisk", "Zerg Broodling", "Zerg Drone", "Zerg Overlord", "Zerg Mutalisk", "Zerg Guardian", "Zerg Queen", "Zerg Defiler", "Zerg Scourge", "Torrarsque (Ultralisk)", "Matriarch (Queen)", "Infested Terran", "Infested Kerrigan", "Unclean One (Defiler)", "Hunter Killer (Hydralisk)", "Devouring One (Zergling)", "Kukulza (Mutalisk)", "Kukulza (Guardian)", "Yggdrasill (Overlord)", "Terran Valkyrie Frigate", "Mutalisk/Guardian Cocoon", "Protoss Corsair", "Protoss Dark Templar(Unit)", "Zerg Devourer", "Protoss Dark Archon", "Protoss Probe", "Protoss Zealot", "Protoss Dragoon", "Protoss High Templar", "Protoss Archon", "Protoss Shuttle", "Protoss Scout", "Protoss Arbiter", "Protoss Carrier", "Protoss Interceptor", "Dark Templar(Hero)", "Zeratul (Dark Templar)", "Tassadar/Zeratul (Archon)", "Fenix (Zealot)", "Fenix (Dragoon)", "Tassadar (Templar)", "Mojo (Scout)", "Warbringer (Reaver)", "Gantrithor (Carrier)", "Protoss Reaver", "Protoss Observer", "Protoss Scarab", "Danimoth (Arbiter)", "Aldaris (Templar)", "Artanis (Scout)", "Rhynadon (Badlands Critter)", "Bengalaas (Jungle Critter)", "Unused - Was Cargo Ship", "Unused - Was Mercenary Gunship", "Scantid (Desert Critter)", "Kakaru (Twilight Critter)", "Ragnasaur (Ashworld Critter)", "Ursadon (Ice World Critter)", "Lurker Egg", "Raszagal", "Samir Duran (Ghost)", "Alexei Stukov (Ghost)", "Map Revealer", "Gerard DuGalle", "Zerg Lurker", "Infested Duran", "Disruption Web", "Terran Command Center", "Terran Comsat Station", "Terran Nuclear Silo", "Terran Supply Depot", "Terran Refinery", "Terran Barracks", "Terran Academy", "Terran Factory", "Terran Starport", "Terran Control Tower", "Terran Science Facility", "Terran Covert Ops", "Terran Physics Lab", "Unused - Was Starbase?", "Terran Machine Shop", "Unused - Was Repair Bay?", "Terran Engineering Bay", "Terran Armory", "Terran Missile Turret", "Terran Bunker", "Norad II", "Ion Cannon", "Uraj Crystal", "Khalis Crystal", "Infested Command Center", "Zerg Hatchery", "Zerg Lair", "Zerg Hive", "Zerg Nydus Canal", "Zerg Hydralisk Den", "Zerg Defiler Mound", "Zerg Greater Spire", "Zerg Queen's Nest", "Zerg Evolution Chamber", "Zerg Ultralisk Cavern", "Zerg Spire", "Zerg Spawning Pool", "Zerg Creep Colony", "Zerg Spore Colony", "Unused Zerg Building", "Zerg Sunken Colony", "Zerg Overmind (With Shell)", "Zerg Overmind", "Zerg Extractor", "Mature Chrysalis", "Zerg Cerebrate", "Zerg Cerebrate Daggoth", "Unused Zerg Building 5", "Protoss Nexus", "Protoss Robotics Facility", "Protoss Pylon", "Protoss Assimilator", "Unused Protoss Building", "Protoss Observatory", "Protoss Gateway", "Unused Protoss Building", "Protoss Photon Cannon", "Protoss Citadel of Adun", "Protoss Cybernetics Core", "Protoss Templar Archives", "Protoss Forge", "Protoss Stargate", "Stasis Cell/Prison", "Protoss Fleet Beacon", "Protoss Arbiter Tribunal", "Protoss Robotics Support Bay", "Protoss Shield Battery", "Khaydarin Crystal Formation", "Protoss Temple", "Xel'Naga Temple", "Mineral Field (Type 1)", "Mineral Field (Type 2)", "Mineral Field (Type 3)", "Cave", "Cave-in", "Cantina", "Mining Platform", "Independant Command Center", "Independant Starport", "Independant Jump Gate", "Ruins", "Kyadarin Crystal Formation", "Vespene Geyser", "Warp Gate", "PSI Disruptor", "Zerg Marker", "Terran Marker", "Protoss Marker", "Zerg Beacon", "Terran Beacon", "Protoss Beacon", "Zerg Flag Beacon", "Terran Flag Beacon", "Protoss Flag Beacon", "Power Generator", "Overmind Cocoon", "Dark Swarm", "Floor Missile Trap", "Floor Hatch", "Left Upper Level Door", "Right Upper Level Door", "Left Pit Door", "Right Pit Door", "Floor Gun Trap", "Left Wall Missile Trap", "Left Wall Flame Trap", "Right Wall Missile Trap", "Right Wall Flame Trap", "Start Location", "Flag", "Young Chrysalis", "Psi Emitter", "Data Disc", "Khaydarin Crystal", "Mineral Cluster Type 1", "Mineral Cluster Type 2", "Protoss Vespene Gas Orb Type 1", "Protoss Vespene Gas Orb Type 2", "Zerg Vespene Gas Sac Type 1", "Zerg Vespene Gas Sac Type 2", "Terran Vespene Gas Tank Type 1", "Terran Vespene Gas Tank Type 2", "None", "Any Unit", "Men", "Building", "Factories" };
		this->printArray(wb, error, 229, (char**)values);
	}
};

class FIELDTYPE_LOCATION : public FIELDTYPE {
public:

	FIELDTYPE_LOCATION(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		if (this->value == 64) {
			PRINT_R("TRIGGERS", "Anywhere");
		}
		else {
			PRINT_R("TRIGGERS", "Location %d \"%s\"", this->value, STR->getRawString(this->value));
		}
	}
};

class FIELDTYPE_STRING : public FIELDTYPE {
public:

	FIELDTYPE_STRING(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		if (this->value == 0) {
			PRINT_R("TRIGGERS", "No string");
		}
		else {
			PRINT_R("TRIGGERS", "String %d \"%s\"", this->value, STR->getRawString(this->value));
		}
	}
};

class FIELDTYPE_SWITCHNAME : public FIELDTYPE {
public:

	FIELDTYPE_SWITCHNAME(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		PRINT_R("TRIGGERS", "Switch %d \"%s\"", this->value, STR->getRawString(this->value));
	}
};

class FIELDTYPE_UNITPROPERTY : public FIELDTYPE {
public:

	FIELDTYPE_UNITPROPERTY(unsigned int value) : FIELDTYPE(value) {}
	void print(Section_STR_* STR, WriteBuffer* wb, bool* error) {
		PRINT_R("TRIGGERS", "Unit properties %d", this->value);
	}
};



class TriggerCondition : public TriggerContents {
public:

	TriggerCondition(unsigned int index, const char* name) : TriggerCondition(index, name, nullptr) { this->dataCount = 0; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1) : TriggerCondition(index, name, data1, nullptr) { this->dataCount = 1; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2) : TriggerCondition(index, name, data1, data2, nullptr) { this->dataCount = 2; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3) : TriggerCondition(index, name, data1, data2, data3, nullptr) { this->dataCount = 3; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4) : TriggerCondition(index, name, data1, data2, data3, data4, nullptr) { this->dataCount = 4; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5) : TriggerCondition(index, name, data1, data2, data3, data4, data5, nullptr) { this->dataCount = 5; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6) : TriggerCondition(index, name, data1, data2, data3, data4, data5, data6, nullptr) { this->dataCount = 6; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7) : TriggerCondition(index, name, data1, data2, data3, data4, data5, data6, data7, nullptr) { this->dataCount = 7; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7, FIELDTYPE* data8) : TriggerCondition(index, name, data1, data2, data3, data4, data5, data6, data7, data8, nullptr) { this->dataCount = 8; }
	TriggerCondition(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7, FIELDTYPE* data8, FIELDTYPE* data9) {
		this->data1 = data1;
		this->data2 = data2;
		this->data3 = data3;
		this->data4 = data4;
		this->data5 = data5;
		this->data6 = data6;
		this->data7 = data7;
		this->data8 = data8;
		this->data9 = data9;
		this->dataCount = 9;
		this->name = name;
		const char* templates[] = { "NoCondition();", "CountdownTimer(Comparison, Time);", "Command(Player, Comparison, Number, Unit);", "Bring(Player, Comparison, Number, Unit, Location);", "Accumulate(Player, Comparison, Number, ResourceType);", "Kills(Player, Comparison, Number, Unit);", "CommandMost(Unit);", "CommandMostAt(Unit, Location);", "MostKills(Unit);", "HighestScore(ScoreType);", "MostResources(ResourceType);", "Switch(Switch, State);", "ElapsedTime(Comparison, Time);", "Briefing();", "Opponents(Player, Comparison, Number);", "Deaths(Player, Comparison, Number, Unit);", "CommandLeast(Unit);", "CommandLeastAt(Unit, Location);", "LeastKills(Unit);", "LowestScore(ScoreType);", "LeastResources(ResourceType);", "Score(Player, ScoreType, Comparison, Number);", "Always();", "Never();" };
		this->templateStr = templates[index];
		this->invalidTemplateStr = "InvalidAction(data1, data2, data3, data4, data5, data6, data7, data8, data9);";
	}

	static TriggerCondition* get(Condition* rawCond);

};

class TriggerAction : public TriggerContents {
public:

	TriggerAction(unsigned int index, const char* name) : TriggerAction(index, name, nullptr) { this->dataCount = 0; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1) : TriggerAction(index, name, data1, nullptr) { this->dataCount = 1; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2) : TriggerAction(index, name, data1, data2, nullptr) { this->dataCount = 2; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3) : TriggerAction(index, name, data1, data2, data3, nullptr) { this->dataCount = 3; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4) : TriggerAction(index, name, data1, data2, data3, data4, nullptr) { this->dataCount = 4; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5) : TriggerAction(index, name, data1, data2, data3, data4, data5, nullptr) { this->dataCount = 5; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6) : TriggerAction(index, name, data1, data2, data3, data4, data5, data6, nullptr) { this->dataCount = 6; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7) : TriggerAction(index, name, data1, data2, data3, data4, data5, data6, data7, nullptr) { this->dataCount = 7; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7, FIELDTYPE* data8) : TriggerAction(index, name, data1, data2, data3, data4, data5, data6, data7, data8, nullptr) { this->dataCount = 8; }
	TriggerAction(unsigned int index, const char* name, FIELDTYPE* data1, FIELDTYPE* data2, FIELDTYPE* data3, FIELDTYPE* data4, FIELDTYPE* data5, FIELDTYPE* data6, FIELDTYPE* data7, FIELDTYPE* data8, FIELDTYPE* data9) {
		this->data1 = data1;
		this->data2 = data2;
		this->data3 = data3;
		this->data4 = data4;
		this->data5 = data5;
		this->data6 = data6;
		this->data7 = data7;
		this->data8 = data8;
		this->data9 = data9;
		this->dataCount = 9;
		this->name = name;
		const char* templates[] = { "NoAction();", "Victory();", "Defeat();", "PreserveTrigger();", "Wait(Time);", "PauseGame();", "UnpauseGame();", "Transmission(Unit, Where, WAVName, TimeModifier, Time, Text, AlwaysDisplay);", "PlayWAV(WAVName);", "DisplayText(Text, AlwaysDisplay);", "CenterView(Where);", "CreateUnitWithProperties(Count, Unit, Where, Player, Properties);", "SetMissionObjectives(Text);", "SetSwitch(Switch, State);", "SetCountdownTimer(TimeModifier, Time);", "RunAIScript(Script);", "RunAIScriptAt(Script, Where);", "LeaderBoardControl(Unit, Label);", "LeaderBoardControlAt(Unit, Location, Label);", "LeaderBoardResources(ResourceType, Label);", "LeaderBoardKills(Unit, Label);", "LeaderBoardScore(ScoreType, Label);", "KillUnit(Unit, Player);", "KillUnitAt(Count, Unit, Where, ForPlayer);", "RemoveUnit(Unit, Player);", "RemoveUnitAt(Count, Unit, Where, ForPlayer);", "SetResources(Player, Modifier, Amount, ResourceType);", "SetScore(Player, Modifier, Amount, ScoreType);", "MinimapPing(Where);", "TalkingPortrait(Unit, Time);", "MuteUnitSpeech();", "UnMuteUnitSpeech();", "LeaderBoardComputerPlayers(State);", "LeaderBoardGoalControl(Goal, Unit, Label);", "LeaderBoardGoalControlAt(Goal, Unit, Location, Label);", "LeaderBoardGoalResources(Goal, ResourceType, Label);", "LeaderBoardGoalKills(Goal, Unit, Label);", "LeaderBoardGoalScore(Goal, ScoreType, Label);", "MoveLocation(Location, OnUnit, Owner, DestLocation);", "MoveUnit(Count, UnitType, Owner, StartLocation, DestLocation);", "LeaderBoardGreed(Goal);", "SetNextScenario(ScenarioName);", "SetDoodadState(State, Unit, Owner, Where);", "SetInvincibility(State, Unit, Owner, Where);", "CreateUnit(Number, Unit, Where, ForPlayer);", "SetDeaths(Player, Modifier, Number, Unit);", "Order(Unit, Owner, StartLocation, OrderType, DestLocation);", "Comment(Text);", "GiveUnits(Count, Unit, Owner, Where, NewOwner);", "ModifyUnitHitPoints(Count, Unit, Owner, Where, Percent);", "ModifyUnitEnergy(Count, Unit, Owner, Where, Percent);", "ModifyUnitShields(Count, Unit, Owner, Where, Percent);", "ModifyUnitResourceAmount(Count, Owner, Where, NewValue);", "ModifyUnitHangarCount(Add, Count, Unit, Owner, Where);", "PauseTimer();", "UnpauseTimer();", "Draw();", "SetAllianceStatus(Player, Status);" };
		this->invalidTemplateStr = "InvalidAction(data1, data2, data3, data4, data5, data6, data7, data8, data9);";
		this->templateStr = templates[index];
	}


	static TriggerAction* get(Action* rawAct);
};

class CONDITION_INVALIDCONDITION : public TriggerCondition {
public:
	CONDITION_INVALIDCONDITION(Condition* rawCond) : TriggerCondition(-1, "InvalidCondition", new FIELDTYPE_NUMBER(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->ConditionType), new FIELDTYPE_NUMBER(rawCond->Flags), new FIELDTYPE_NUMBER(rawCond->groupNumber), new FIELDTYPE_NUMBER(rawCond->locationNumber), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_NUMBER(rawCond->Resource), new FIELDTYPE_NUMBER(rawCond->UnitID), new FIELDTYPE_NUMBER(rawCond->Unused)) {
		this->templateStr = nullptr;
	}
};

class CONDITION_NOCONDITION : public TriggerCondition {
public:
	CONDITION_NOCONDITION(Condition* rawCond) : TriggerCondition(0, "NoCondition") {}
};

class CONDITION_COUNTDOWNTIMER : public TriggerCondition {
public:

	CONDITION_COUNTDOWNTIMER(Condition* rawCond) : TriggerCondition(1, "CountdownTimer", new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_COMPARISON(rawCond->Comparision)) {}
};

class CONDITION_COMMAND : public TriggerCondition {
public:

	CONDITION_COMMAND(Condition* rawCond) : TriggerCondition(2, "Command", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_BRING : public TriggerCondition {
public:

	CONDITION_BRING(Condition* rawCond) : TriggerCondition(3, "Bring", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_UNIT(rawCond->UnitID), new FIELDTYPE_LOCATION(rawCond->locationNumber)) {}
};

class CONDITION_ACCUMULATE : public TriggerCondition {
public:

	CONDITION_ACCUMULATE(Condition* rawCond) : TriggerCondition(4, "Accumulate", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_RESOURCE(rawCond->Resource)) {}
};

class CONDITION_KILL : public TriggerCondition {
public:

	CONDITION_KILL(Condition* rawCond) : TriggerCondition(5, "Kills", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_COMMADNTHEMOST : public TriggerCondition {
public:

	CONDITION_COMMADNTHEMOST(Condition* rawCond) : TriggerCondition(6, "CommandMost", new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_COMMANDSTHEMOSTAT : public TriggerCondition {
public:

	CONDITION_COMMANDSTHEMOSTAT(Condition* rawCond) : TriggerCondition(7, "CommandMostAt", new FIELDTYPE_UNIT(rawCond->UnitID), new FIELDTYPE_LOCATION(rawCond->locationNumber)) {}
};

class CONDITION_MOSTKILLS : public TriggerCondition {
public:

	CONDITION_MOSTKILLS(Condition* rawCond) : TriggerCondition(8, "MostKills", new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_HIGHESTSCORE : public TriggerCondition {
public:

	CONDITION_HIGHESTSCORE(Condition* rawCond) : TriggerCondition(9, "HighestScore", new FIELDTYPE_SCORE(rawCond->Resource)) {}
};

class CONDITION_MOSTRESOURCES : public TriggerCondition {
public:

	CONDITION_MOSTRESOURCES(Condition* rawCond) : TriggerCondition(10, "MostResources", new FIELDTYPE_RESOURCE(rawCond->Resource)) {}
};

class CONDITION_SWITCH : public TriggerCondition {
public:

	CONDITION_SWITCH(Condition* rawCond) : TriggerCondition(11, "Switch", new FIELDTYPE_SWITCHNAME(rawCond->Resource), new FIELDTYPE_SWITCHSTATE(rawCond->Comparision)) {}
};

class CONDITION_ELAPSEDTIME : public TriggerCondition {
public:

	CONDITION_ELAPSEDTIME(Condition* rawCond) : TriggerCondition(12, "ElapsedTime", new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier)) {}
};

class CONDITION_NACD : public TriggerCondition {
public:

	CONDITION_NACD(Condition* rawCond) : TriggerCondition(13, "Briefing") {}
};

class CONDITION_OPPONENTS : public TriggerCondition {
public:

	CONDITION_OPPONENTS(Condition* rawCond) : TriggerCondition(14, "Opponents", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier)) {}
};

class CONDITION_DEATHS : public TriggerCondition {
public:

	CONDITION_DEATHS(Condition* rawCond) : TriggerCondition(15, "Deaths", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier), new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_COMMANDTHELEAST : public TriggerCondition {
public:

	CONDITION_COMMANDTHELEAST(Condition* rawCond) : TriggerCondition(16, "CommandLeast", new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_COMMANDTHELEASTAT : public TriggerCondition {
public:

	CONDITION_COMMANDTHELEASTAT(Condition* rawCond) : TriggerCondition(17, "CommandLeastAt", new FIELDTYPE_UNIT(rawCond->UnitID), new FIELDTYPE_LOCATION(rawCond->locationNumber)) {}
};

class CONDITION_LEASTKILLS : public TriggerCondition {
public:

	CONDITION_LEASTKILLS(Condition* rawCond) : TriggerCondition(18, "LeastKills", new FIELDTYPE_UNIT(rawCond->UnitID)) {}
};

class CONDITION_LOWESTSCORE : public TriggerCondition {
public:

	CONDITION_LOWESTSCORE(Condition* rawCond) : TriggerCondition(19, "LowestScore", new FIELDTYPE_SCORE(rawCond->Resource)) {}
};

class CONDITION_LEASTRESOURCES : public TriggerCondition {
public:

	CONDITION_LEASTRESOURCES(Condition* rawCond) : TriggerCondition(20, "LeastResources", new FIELDTYPE_RESOURCE(rawCond->Resource)) {}
};

class CONDITION_SCORE : public TriggerCondition {
public:

	CONDITION_SCORE(Condition* rawCond) : TriggerCondition(21, "Score", new FIELDTYPE_PLAYER(rawCond->groupNumber), new FIELDTYPE_SCORE(rawCond->Resource), new FIELDTYPE_COMPARISON(rawCond->Comparision), new FIELDTYPE_NUMBER(rawCond->Quantifier)) {}
};

class CONDITION_ALWAYS : public TriggerCondition {
public:

	CONDITION_ALWAYS(Condition* rawCond) : TriggerCondition(22, "Always") {}
};

class CONDITION_NEVER : public TriggerCondition {
public:

	CONDITION_NEVER(Condition* rawCond) : TriggerCondition(23, "Never") {}
};


class ACTION_INVALIDACTION : public TriggerAction {
public:
	ACTION_INVALIDACTION(Action* rawAct) : TriggerAction(-1, "InvalidAction") {
		this->templateStr = nullptr;
	}
};

class ACTION_NOACTION : public TriggerAction {
public:

	ACTION_NOACTION(Action* rawAct) : TriggerAction(0, "NoAction") {}
};

class ACTION_VICTORY : public TriggerAction {
public:

	ACTION_VICTORY(Action* rawAct) : TriggerAction(1, "Victory") {}
};

class ACTION_DEFEAT : public TriggerAction {
public:

	ACTION_DEFEAT(Action* rawAct) : TriggerAction(2, "Defeat") {}
};

class ACTION_PRESERVETRIGGER : public TriggerAction {
public:

	ACTION_PRESERVETRIGGER(Action* rawAct) : TriggerAction(3, "PreserveTrigger") {}
};

class ACTION_WAIT : public TriggerAction {
public:

	ACTION_WAIT(Action* rawAct) : TriggerAction(4, "Wait", new FIELDTYPE_NUMBER(rawAct->Time)) {}
};

class ACTION_PAUSEGAME : public TriggerAction {
public:

	ACTION_PAUSEGAME(Action* rawAct) : TriggerAction(5, "PauseGame") {}
};

class ACTION_UNPAUSEGAME : public TriggerAction {
public:

	ACTION_UNPAUSEGAME(Action* rawAct) : TriggerAction(6, "UnpauseGame") {}
};

class ACTION_TRANSMISSION : public TriggerAction {
public:

	ACTION_TRANSMISSION(Action* rawAct) : TriggerAction(7, "Transmission", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_STRING(rawAct->WAVStringNumber), new FIELDTYPE_MODIFIER(rawAct->UnitsNumber), new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_STRING(rawAct->TriggerText), new FIELDTYPE_NUMBER(rawAct->Flags)) {}
};

class ACTION_PLAYWAV : public TriggerAction {
public:

	ACTION_PLAYWAV(Action* rawAct) : TriggerAction(8, "PlayWAV", new FIELDTYPE_STRING(rawAct->WAVStringNumber)) {}
};

class ACTION_DISPLAYTEXTMESSAGE : public TriggerAction {
public:

	ACTION_DISPLAYTEXTMESSAGE(Action* rawAct) : TriggerAction(9, "DisplayText", new FIELDTYPE_STRING(rawAct->TriggerText), new FIELDTYPE_NUMBER(rawAct->Flags)) {}
};

class ACTION_CENTERVIEW : public TriggerAction {
public:

	ACTION_CENTERVIEW(Action* rawAct) : TriggerAction(10, "CenterView", new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_CREATEUNITWITHPROPERTIES : public TriggerAction {
public:

	ACTION_CREATEUNITWITHPROPERTIES(Action* rawAct) : TriggerAction(11, "CreateUnitWithProperties", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_UNITPROPERTY(rawAct->Group)) {}
};

class ACTION_SETMISSIONOBJECTIVES : public TriggerAction {
public:

	ACTION_SETMISSIONOBJECTIVES(Action* rawAct) : TriggerAction(12, "SetMissionObjectives", new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_SETSWITCH : public TriggerAction {
public:

	ACTION_SETSWITCH(Action* rawAct) : TriggerAction(13, "SetSwitch", new FIELDTYPE_SWITCHNAME(rawAct->Group), new FIELDTYPE_SWITCHACTION(rawAct->UnitsNumber)) {}
};

class ACTION_SETCOUNTDOWNTIMER : public TriggerAction {
public:

	ACTION_SETCOUNTDOWNTIMER(Action* rawAct) : TriggerAction(14, "SetCountdownTimer", new FIELDTYPE_MODIFIER(rawAct->UnitsNumber), new FIELDTYPE_NUMBER(rawAct->Time)) {}
};

class ACTION_RUNAISCRIPT : public TriggerAction {
public:

	ACTION_RUNAISCRIPT(Action* rawAct) : TriggerAction(15, "RunAIScript", new FIELDTYPE_AISCRIPT(rawAct->Group)) {}
};

class ACTION_RUNAISCRIPTATLOCATION : public TriggerAction {
public:

	ACTION_RUNAISCRIPTATLOCATION(Action* rawAct) : TriggerAction(16, "RunAIScriptAt", new FIELDTYPE_AISCRIPT(rawAct->Group), new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_LEADERBOARDCONTROL : public TriggerAction {
public:

	ACTION_LEADERBOARDCONTROL(Action* rawAct) : TriggerAction(17, "LeaderBoardControl", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDCONTROLATLOCATION : public TriggerAction {
public:

	ACTION_LEADERBOARDCONTROLATLOCATION(Action* rawAct) : TriggerAction(18, "LeaderBoardControlAt", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDRECOURCES : public TriggerAction {
public:

	ACTION_LEADERBOARDRECOURCES(Action* rawAct) : TriggerAction(19, "LeaderBoardResources", new FIELDTYPE_RESOURCE(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDKILLS : public TriggerAction {
public:

	ACTION_LEADERBOARDKILLS(Action* rawAct) : TriggerAction(20, "LeaderBoardKills", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDPOINTS : public TriggerAction {
public:

	ACTION_LEADERBOARDPOINTS(Action* rawAct) : TriggerAction(21, "LeaderBoardScore", new FIELDTYPE_SCORE(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_KILLUNIT : public TriggerAction {
public:

	ACTION_KILLUNIT(Action* rawAct) : TriggerAction(22, "KillUnit", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player)) {}
};

class ACTION_KILLUNITATLOCATION : public TriggerAction {
public:

	ACTION_KILLUNITATLOCATION(Action* rawAct) : TriggerAction(23, "KillUnitAt", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_PLAYER(rawAct->Player)) {}
};

class ACTION_REMOVEUNIT : public TriggerAction {
public:

	ACTION_REMOVEUNIT(Action* rawAct) : TriggerAction(24, "RemoveUnit", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player)) {}
};

class ACTION_REMOVEUNITATLOCATION : public TriggerAction {
public:

	ACTION_REMOVEUNITATLOCATION(Action* rawAct) : TriggerAction(25, "RemoveUnitAt", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_PLAYER(rawAct->Player)) {}
};

class ACTION_SETRESOURCES : public TriggerAction {
public:

	ACTION_SETRESOURCES(Action* rawAct) : TriggerAction(26, "SetResources", new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_MODIFIER(rawAct->UnitsNumber), new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_RESOURCE(rawAct->UnitType)) {}
};

class ACTION_SETSCORE : public TriggerAction {
public:

	ACTION_SETSCORE(Action* rawAct) : TriggerAction(27, "SetScore", new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_MODIFIER(rawAct->UnitsNumber), new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_SCORE(rawAct->UnitType)) {}
};

class ACTION_MINIMAPPING : public TriggerAction {
public:

	ACTION_MINIMAPPING(Action* rawAct) : TriggerAction(28, "MinimapPing", new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_TALKINGPORTRAIT : public TriggerAction {
public:

	ACTION_TALKINGPORTRAIT(Action* rawAct) : TriggerAction(29, "TalkingPortrait", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_NUMBER(rawAct->Time)) {}
};

class ACTION_MUTEUNITSPEECH : public TriggerAction {
public:

	ACTION_MUTEUNITSPEECH(Action* rawAct) : TriggerAction(30, "MuteUnitSpeech") {}
};

class ACTION_UNMUTEUNITSPEECH : public TriggerAction {
public:

	ACTION_UNMUTEUNITSPEECH(Action* rawAct) : TriggerAction(31, "UnMuteUnitSpeech") {}
};

class ACTION_LEADERBOARDCOMPUTERPLAYERS : public TriggerAction {
public:

	ACTION_LEADERBOARDCOMPUTERPLAYERS(Action* rawAct) : TriggerAction(32, "LeaderBoardComputerPlayers", new FIELDTYPE_PROPSTATE(rawAct->UnitsNumber)) {}
};

class ACTION_LEADERBOARDGOALCONTROL : public TriggerAction {
public:

	ACTION_LEADERBOARDGOALCONTROL(Action* rawAct) : TriggerAction(33, "LeaderBoardGoalControl", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDGOALCONTROLATLOCATION : public TriggerAction {
public:

	ACTION_LEADERBOARDGOALCONTROLATLOCATION(Action* rawAct) : TriggerAction(34, "LeaderBoardGoalControlAt", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDGOALRESOURCES : public TriggerAction {
public:

	ACTION_LEADERBOARDGOALRESOURCES(Action* rawAct) : TriggerAction(35, "LeaderBoardGoalResources", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_RESOURCE(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDGOALKILLS : public TriggerAction {
public:

	ACTION_LEADERBOARDGOALKILLS(Action* rawAct) : TriggerAction(36, "LeaderBoardGoalKills", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_LEADERBOARDGOALPOINTS : public TriggerAction {
public:

	ACTION_LEADERBOARDGOALPOINTS(Action* rawAct) : TriggerAction(37, "LeaderBoardGoalScore", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_SCORE(rawAct->UnitType), new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_MOVELOCATION : public TriggerAction {
public:

	ACTION_MOVELOCATION(Action* rawAct) : TriggerAction(38, "MoveLocation", new FIELDTYPE_LOCATION(rawAct->Group), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_MOVEUNIT : public TriggerAction {
public:

	ACTION_MOVEUNIT(Action* rawAct) : TriggerAction(39, "MoveUnit", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_LOCATION(rawAct->Group)) {}
};

class ACTION_LEADERBOARDGREED : public TriggerAction {
public:

	ACTION_LEADERBOARDGREED(Action* rawAct) : TriggerAction(40, "LeaderBoardGreed", new FIELDTYPE_NUMBER(rawAct->Group)) {}
};

class ACTION_SETNEXTSCENARIO : public TriggerAction {
public:

	ACTION_SETNEXTSCENARIO(Action* rawAct) : TriggerAction(41, "SetNextScenario", new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_SETDOODADSTATE : public TriggerAction {
public:

	ACTION_SETDOODADSTATE(Action* rawAct) : TriggerAction(42, "SetDoodadState", new FIELDTYPE_PROPSTATE(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_SETINVINCIBILLITY : public TriggerAction {
public:

	ACTION_SETINVINCIBILLITY(Action* rawAct) : TriggerAction(43, "SetInvincibility", new FIELDTYPE_PROPSTATE(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_CREATEUNIT : public TriggerAction {
public:

	ACTION_CREATEUNIT(Action* rawAct) : TriggerAction(44, "CreateUnit", new FIELDTYPE_NUMBER(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_PLAYER(rawAct->Player)) {}
};

class ACTION_SETDEATHS : public TriggerAction {
public:

	ACTION_SETDEATHS(Action* rawAct) : TriggerAction(45, "SetDeaths", new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_MODIFIER(rawAct->UnitsNumber), new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_UNIT(rawAct->UnitType)) {}
};

class ACTION_ORDER : public TriggerAction {
public:

	ACTION_ORDER(Action* rawAct) : TriggerAction(46, "Order", new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_ORDER(rawAct->UnitsNumber), new FIELDTYPE_LOCATION(rawAct->Group)) {}
};

class ACTION_COMMENT : public TriggerAction {
public:

	ACTION_COMMENT(Action* rawAct) : TriggerAction(47, "Comment", new FIELDTYPE_STRING(rawAct->TriggerText)) {}
};

class ACTION_GIVEUNITSTOPLAYER : public TriggerAction {
public:

	ACTION_GIVEUNITSTOPLAYER(Action* rawAct) : TriggerAction(48, "GiveUnits", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_PLAYER(rawAct->Group)) {}
};

class ACTION_MODIFYUNITHITPOINTS : public TriggerAction {
public:

	ACTION_MODIFYUNITHITPOINTS(Action* rawAct) : TriggerAction(49, "ModifyUnitHitPoints", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_NUMBER(rawAct->Group)) {}
};

class ACTION_MODIFYUNITENERGY : public TriggerAction {
public:

	ACTION_MODIFYUNITENERGY(Action* rawAct) : TriggerAction(50, "ModifyUnitEnergy", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_NUMBER(rawAct->Group)) {}
};

class ACTION_MODIFYUNITSHIELDPOINTS : public TriggerAction {
public:

	ACTION_MODIFYUNITSHIELDPOINTS(Action* rawAct) : TriggerAction(51, "ModifyUnitShields", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_NUMBER(rawAct->Group)) {}
};

class ACTION_MODIFYUNITRESOURCEAMOUNT : public TriggerAction {
public:

	ACTION_MODIFYUNITRESOURCEAMOUNT(Action* rawAct) : TriggerAction(52, "ModifyUnitResourceAmount", new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation), new FIELDTYPE_NUMBER(rawAct->Group)) {}
};

class ACTION_MODIFYUNITHANGERCOUNT : public TriggerAction {
public:

	ACTION_MODIFYUNITHANGERCOUNT(Action* rawAct) : TriggerAction(53, "ModifyUnitHangarCount", new FIELDTYPE_NUMBER(rawAct->Group), new FIELDTYPE_COUNT(rawAct->UnitsNumber), new FIELDTYPE_UNIT(rawAct->UnitType), new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_LOCATION(rawAct->SourceLocation)) {}
};

class ACTION_PAUSETIMER : public TriggerAction {
public:

	ACTION_PAUSETIMER(Action* rawAct) : TriggerAction(54, "PauseTimer") {}
};

class ACTION_UNPAUSETIMER : public TriggerAction {
public:

	ACTION_UNPAUSETIMER(Action* rawAct) : TriggerAction(55, "UnpauseTimer") {}
};

class ACTION_DRAW : public TriggerAction {
public:

	ACTION_DRAW(Action* rawAct) : TriggerAction(56, "Draw") {}
};

class ACTION_SETALLIANCESTATUS : public TriggerAction {
public:

	ACTION_SETALLIANCESTATUS(Action* rawAct) : TriggerAction(57, "SetAllianceStatus", new FIELDTYPE_PLAYER(rawAct->Player), new FIELDTYPE_ALLYSTATUS(rawAct->UnitType)) {}
};

#endif
#endif