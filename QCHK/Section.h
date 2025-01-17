#pragma once

#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "common.h"

//#define USE_DEFUC_SANC
//#define TRIM_STR

class Section {
public:
	Section(unsigned char* name, unsigned int size, ReadBuffer* buffer, bool isSanc);
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

	bool isSanc = false;

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

public:

	COMMON_CONSTR_SEC(Section_STR_)

	virtual ~Section_STR_() {}

	virtual unsigned int getNewStringIndex(char* string, bool* error) = 0;

	virtual char* getRawString(unsigned int index) = 0;

	virtual void setRawString(unsigned int index, char* string) = 0;

	virtual unsigned int getLastStringIndex() = 0;
};

class Section_STR_Native : public Section_STR_ {

public:
	COMMON_CONSTR_SEC_DER(Section_STR_Native, Section_STR_)

public:

	virtual unsigned int getLastStringIndex() {
		return strings.getSize();
	}

	unsigned int getNewStringIndex(char* string, bool* error) {
		for (unsigned int i = 0; i < this->strings.getSize(); i++) {
			char* str = strings[i];
			if (!strcmp(string, str)) {
				return i + 1;
			}
		}
		GET_CLONED_STRING(newString, string, { *error = true; return 0; });
		if (!strings.append(newString)) {
			*error = true;
			return false;
		}
		return strings.getSize();
	}

	virtual ~Section_STR_Native() {
		for (unsigned int i = 0; i < strings.getSize(); i++) {
			char* str = strings[i];
			free(str);
		}
	}

	bool parse() {
		
		bool error = false;
		unsigned int totalStrings = this->buffer->readShort(&error);
		if (!isSanc) { // Sanc must be trusted, this is critical
			totalStrings = 0xffff; // Don't trust the above
		}
		if (error) {
			return false;
		}

		unsigned int plannedEnd = 2 + (totalStrings * 2); // Where the offsets should end and data begin
		Array<unsigned short> offsets;

		for (unsigned int i = 0; i < totalStrings; i++) {
			unsigned short offset = this->buffer->readShort(&error);
			if (error) {
				return false;
			}
			
			if (offset < plannedEnd && !isSanc) { // Total strings was a lie, recalculate
				unsigned int maxStrings = (offset - 2) / 2;
				if (totalStrings > maxStrings) {
					totalStrings = maxStrings;
					plannedEnd = 2 + (totalStrings * 2);
				}
			}
			
			if (!offsets.append(offset)) {
				return false;
			}

		}

		totalStrings = offsets.getSize();
		unsigned int dataLength = this->size - (2 + (2 * totalStrings));
		unsigned char* data = this->buffer->readArray(dataLength, &error);
		unsigned int off = 2 + (2 * totalStrings);
		for (unsigned int i = 0; i < totalStrings; i++) {
			unsigned short offset = offsets.get(i);
			unsigned char* str = (data + (offset - off));
			if (offset < off || offset - off > dataLength) {
				continue;
			}
			
			GET_CLONED_STRING(newStr, (char*) str, { free(data); return false; });
			if (!strings.append(newStr)) {
				free(data);
				return false;
			}
		}
		free(data);
		return !error;
	}

	virtual bool write(WriteBuffer* buffer) {
		unsigned int totalStrings = strings.getSize();
		bool error = false;
		buffer->writeShort(totalStrings, &error);
		if (error) {
			return false;
		}
		unsigned short offset = 2 + (2 * totalStrings);
		for (unsigned int i = 0; i < totalStrings; i++) {
			char* str = strings[i];
			buffer->writeShort(offset, &error);
			if (error) { return false; }
			offset += (unsigned short) (strlen(str) + 1);
		}
		for (unsigned int i = 0; i < totalStrings; i++) {
			char* str = strings[i];
			buffer->writeZeroDelimitedString((unsigned char*) str, &error);
			if (error) { return false; }
		}
		return !error;
	}

	virtual char* getRawString(unsigned int index) {
		if (index <= strings.getSize() && index > 0) {
			return strings.get(index - 1);
		}
		return "(null)";
	}

	virtual void setRawString(unsigned int index, char* string) {
		if (index <= strings.getSize() && index > 0) {
			char* current = strings.get(index - 1);
			GET_CLONED_STRING(newStr, string, { return; });
			free(current);
			strings.set(index - 1, newStr);

		}
	}


private:
	Array<char*> strings;

};

#ifdef USE_DEFUC_SANC
class Section_STR_Sanc : public Section_STR_ {

	struct Section_STR_Offset {
		unsigned short offset;
		unsigned short index;
	};

public:

	unsigned int getNewStringIndex(char* string, bool* error) {
		unsigned int off = 2 + (2 * this->totalStrings); // Offset list
		unsigned int freePosition = this->lastUsedOffset;
		unsigned int stringLength = strlen(string);
		this->offsets[this->lastUsedOffsetIndex] = freePosition + off;
		memcpy(&(data[freePosition]), string, stringLength + 1);

		if (freePosition + off + stringLength >= 0x1BA3) {
			*error = true;
			LOG_ERROR("STR", "Failed to insert new string, too many strings");
			return 0;
		}

		this->lastUsedOffsetIndex++;
		this->lastUsedOffset += stringLength + 1; // After \0 delimiter
		LOG("STR", "Allocating new index %d for string \"%s\"", this->lastUsedOffsetIndex, string);
		return this->lastUsedOffsetIndex;
	}

	char* getRawString(unsigned int index) {
		unsigned int off = 2 + (2 * this->offsets.getSize());
		unsigned short offset = this->offsets[index - 1];
		return (char*)(&(this->data[offset - off]));
	}

	void setRawString(unsigned int index, char* string) {
		unsigned int off = 2 + (2 * this->offsets.getSize());
		unsigned int offset = this->offsets[index - 1] - off;
		unsigned int len = strlen(string);
		for (unsigned int i = 0; i <= len; i++) {
			this->data[offset + 1 + i] = string[i];
		}
		this->data[offset + 1 + len] = 0;
	}

	COMMON_CONSTR_SEC_DER(Section_STR_Sanc, Section_STR_)

	virtual ~Section_STR_Sanc() {
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
		isSanc = true;
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
			if (!offsets.append(offset)) {
				return false;
			}

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
		this->dataLength = this->size - (2 + (2 * this->totalStrings));
		this->data = this->buffer->readArray(dataLength, &error);
		return !error;
	}

public:

	virtual bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeShort(this->totalStrings, &error);
		if (error) {
			return false;
		}
		for (unsigned int i = 0; i<totalStrings; i++) {
			buffer->writeShort(this->offsets[i], &error);
			if (error) {
				return false;
			}
		};
		unsigned int rawDataLength = this->size - (2 + (2 * this->totalStrings));
		buffer->writeArray(this->data, rawDataLength, &error);
		return !error;
	}

private:

	unsigned int dataLength;
	Array<unsigned short> offsets;
	Section_STR_Offset* sortedOffsets = nullptr;

	unsigned int lastUsedIndex;
	unsigned int lastUsedOffset;
	unsigned int lastUsedOffsetIndex;
};

#else

class Section_STR_Sanc : public Section_STR_Native {

public:

	COMMON_CONSTR_SEC_DER(Section_STR_Sanc, Section_STR_Native)

	virtual bool parse() {
		bool error = false;
		this->dataSize = this->size;
		this->data = (char*) this->buffer->readArray(this->dataSize, &error);
		return !error;
	}

	virtual bool write(WriteBuffer* wb) {
		WriteBuffer twb;
		if (!Section_STR_Native::write(&twb)) {
			return false;
		}
		unsigned char* rawData;
		unsigned int writtenLength;
		bool error = false;
		twb.getWrittenData(&rawData, &writtenLength);
#ifdef TRIM_STR
		if (writtenLength >= 0x1bb2) {
			writtenLength = 0x1bb0;
			rawData[0x1bb0] = 0;
			rawData[0x1bb1] = 0;
		}
#else		
		if (writtenLength >= 0x1bb2) {
			return false;
		}
#endif
		wb->writeArray(rawData, writtenLength, &error);
		if (error) { return false; }
		wb->writeArray((unsigned char*) (data + writtenLength), dataSize - writtenLength, &error);
		if (error) {return false;}
		return true;
	}

	virtual char* getRawSancString(unsigned int index) {
		unsigned short* offsetPtr = (unsigned short*) (data + 2);
		unsigned short offset = offsetPtr[index - 1];
		char* str = (char*)(data + offset);
		return str;
	}


private:
	char* data;
	unsigned int dataSize;
};
#endif

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

class Section_DIM_ : public Section {

public:

	COMMON_CONSTR_SEC(Section_DIM_)

	bool parse() {
		bool error = false;
		width = this->buffer->readShort(&error);
		if (error) {
			return false;
		}
		height = this->buffer->readShort(&error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeShort(this->width, &error);
		if (error) {
			return false;
		}
		buffer->writeShort(this->height, &error);
		return !error;
	}

	unsigned short width;
	unsigned short height;

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

struct Unit {
	unsigned int instanceID; //  The unit's class instance (sort of a "serial number")
	unsigned short x; //  X coordinate of unit
	unsigned short y; //  Y coordinate of unit
	unsigned short unitID; //  Unit ID
	unsigned short relationType; //  Type of relation to another building (i.e. add-on, nydus link)
	unsigned short flags; //  Flags of special properties which can be applied to the unit and are valid:
	unsigned short validFields; //  Out of the elements of the unit data, the properties which can be changed by the map maker:
	unsigned char ownerID; //  Player number of owner (0-based)
	unsigned char HP; //  Hit points % (1-100)
	unsigned char Shields; //  Shield points % (1-100)
	unsigned char Energy; //  Energy points % (1-100)
	unsigned int Resources; //  Resource amount
	unsigned short Hangar; //  Number of units in hangar
	unsigned short StateFlags; //  Unit state flags
	unsigned int Unused; //  Unused
	unsigned int InstanceRelation; //  Class instance of the unit to which this unit is related to (i.e. via an add-on, nydus link, etc.). It is "0" if the unit is not linked to any other unit.
};

class Section_UNIT : public Section {

public:

	Array<Unit*> units;

	COMMON_CONSTR_SEC(Section_UNIT)

		virtual ~Section_UNIT() {
		for (unsigned int i = 0; i < units.getSize(); i++) {
			if (units[i] != nullptr) {
				Unit* unit = units[i];
				free(unit);
				units[i] = nullptr;
			}
		}
	}

protected:

	bool parse() {

		unsigned int totalUnits = this->size / sizeof(Unit);
		bool error = false;
		for (unsigned int i = 0; i < totalUnits; i++) {
			Unit* unit = (Unit*) this->buffer->readArray(sizeof(Unit), &error);
			if (error) {
				return false;
			}
			if (!this->units.append(unit)) {
				free(unit);
				return false;
			}
		}
		return true;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		for (unsigned int i = 0; i < this->units.getSize(); i++) {
			Unit* unit = this->units[i];
			buffer->writeArray((unsigned char*)unit, sizeof(Unit), &error);
			if (error) {
				return false;
			}
		};
		return true;
	}


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

class Section_MASK : public Section {

public:

	COMMON_CONSTR_SEC(Section_MASK)

	~Section_MASK() {
		if (data != nullptr) {
			free(data);
			data = nullptr;
		}
	}

	bool parse() {
		bool error = false;
		data = this->buffer->readArray(this->size, &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray(this->data, this->size, &error);
		return !error;
	}

	unsigned char* data = nullptr;

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
			if (!this->locations.append(location)) {
				free(location);
				return false;
			}
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

class Section_TRIG : public Section {

public:

	COMMON_CONSTR_SEC(Section_TRIG)

		virtual ~Section_TRIG();

	Array<Trigger*> triggers;

#ifdef TRIG_PRINT

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

protected:

	bool parse() {

		unsigned int totalTriggers = this->size / sizeof(Trigger);
		bool error = false;
		for (unsigned int i = 0; i < totalTriggers; i++) {
			Trigger* trigger = (Trigger*) this->buffer->readArray(sizeof(Trigger), &error);
			if (error) {
				return false;
			}
			if (!this->triggers.append(trigger)) {
				free(trigger);
				return false;
			}
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

#define PRINT_R(ignore, format, ...) {char buffer[2048]; sprintf_s(buffer, format, __VA_ARGS__); wb->writeFixedLengthString((unsigned char*) buffer, error); if(*error) {return;}}

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

};

class Section_MBRF : public Section_TRIG {
public:
	Section_MBRF(unsigned char* name, unsigned int size, ReadBuffer* buffer, bool isSanc) : Section_TRIG(name, size, buffer, isSanc) { }

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

struct Section_UNIS_STRUCTURE {

	unsigned char used[228]; // : 1 byte for each unit, in order of Unit ID
	unsigned int HP[228]; // : Hit points for unit(Note the displayed value is this value / 256, with the low byte being a fractional HP value)
	unsigned short shield[228]; // : Shield points, in order of Unit ID
	unsigned char armor[228]; //: Armor points, in order of Unit ID
	unsigned short build_time[228]; // : Build time(1 / 60 seconds), in order of Unit ID
	unsigned short mineral_cost[228]; // : Mineral cost, in order of Unit ID
	unsigned short gas_cost[228]; // : Gas cost, in order of Unit ID
	unsigned short name[228]; // : String number, in order of Unit ID
	unsigned short damage[100]; // : Base weapon damage the weapon does, in weapon ID order(#List of Unit Weapon IDs)
	unsigned short damage_bonus[100]; // : Upgrade bonus weapon damage, in weapon ID order

};

class Section_UNIS : public Section {

public:

	COMMON_CONSTR_SEC(Section_UNIS)

	Section_UNIS_STRUCTURE* data = nullptr;

	virtual ~Section_UNIS() {
		if (this->data != nullptr) {
			free(this->data);
			this->data = nullptr;
		}
	}

protected:
	bool parse() {
		bool error = false;
		this->data = (Section_UNIS_STRUCTURE*) this->buffer->readArray(sizeof(Section_UNIS_STRUCTURE), &error);
		return !error;
	}

	bool write(WriteBuffer* buffer) {
		bool error = false;
		buffer->writeArray((unsigned char*) this->data, sizeof(Section_UNIS_STRUCTURE), &error);
		return !error;
	}
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
		char* values[] = {
			(char*)(('u' << 24) | ('C' << 16) | ('M' << 8) | ('T')), "Terran Custom Level",
			(char*)(('u' << 24) | ('C' << 16) | ('M' << 8) | ('Z')), "Zerg Custom Level",
			(char*)(('u' << 24) | ('C' << 16) | ('M' << 8) | ('P')), "Protoss Custom Level",
			(char*)(('x' << 24) | ('C' << 16) | ('M' << 8) | ('T')), "Terran Expansion Custom Level",
			(char*)(('x' << 24) | ('C' << 16) | ('M' << 8) | ('Z')), "Zerg Expansion Custom Level",
			(char*)(('x' << 24) | ('C' << 16) | ('M' << 8) | ('P')), "Protoss Expansion Custom Level",
			(char*)(('f' << 24) | ('O' << 16) | ('L' << 8) | ('T')), "Terran Campaign Easy",
			(char*)(('D' << 24) | ('E' << 16) | ('M' << 8) | ('T')), "Terran Campaign Medium",
			(char*)(('f' << 24) | ('I' << 16) | ('H' << 8) | ('T')), "Terran Campaign Difficult",
			(char*)(('P' << 24) | ('U' << 16) | ('S' << 8) | ('T')), "Terran Campaign Insane",
			(char*)(('E' << 24) | ('R' << 16) | ('A' << 8) | ('T')), "Terran Campaign Area Town",
			(char*)(('f' << 24) | ('O' << 16) | ('L' << 8) | ('Z')), "Zerg Campaign Easy",
			(char*)(('D' << 24) | ('E' << 16) | ('M' << 8) | ('Z')), "Zerg Campaign Medium",
			(char*)(('f' << 24) | ('I' << 16) | ('H' << 8) | ('Z')), "Zerg Campaign Difficult",
			(char*)(('P' << 24) | ('U' << 16) | ('S' << 8) | ('Z')), "Zerg Campaign Insane",
			(char*)(('E' << 24) | ('R' << 16) | ('A' << 8) | ('Z')), "Zerg Campaign Area Town",
			(char*)(('f' << 24) | ('O' << 16) | ('L' << 8) | ('P')), "Protoss Campaign Easy",
			(char*)(('D' << 24) | ('E' << 16) | ('M' << 8) | ('P')), "Protoss Campaign Medium",
			(char*)(('f' << 24) | ('I' << 16) | ('H' << 8) | ('P')), "Protoss Campaign Difficult",
			(char*)(('P' << 24) | ('U' << 16) | ('S' << 8) | ('P')), "Protoss Campaign Insane",
			(char*)(('E' << 24) | ('R' << 16) | ('A' << 8) | ('P')), "Protoss Campaign Area Town",
			(char*)(('x' << 24) | ('O' << 16) | ('L' << 8) | ('T')), "Expansion Terran Campaign Easy",
			(char*)(('x' << 24) | ('E' << 16) | ('M' << 8) | ('T')), "Expansion Terran Campaign Medium",
			(char*)(('x' << 24) | ('I' << 16) | ('H' << 8) | ('T')), "Expansion Terran Campaign Difficult",
			(char*)(('x' << 24) | ('U' << 16) | ('S' << 8) | ('T')), "Expansion Terran Campaign Insane",
			(char*)(('x' << 24) | ('R' << 16) | ('A' << 8) | ('T')), "Expansion Terran Campaign Area Town",
			(char*)(('x' << 24) | ('O' << 16) | ('L' << 8) | ('Z')), "Expansion Zerg Campaign Easy",
			(char*)(('x' << 24) | ('E' << 16) | ('M' << 8) | ('Z')), "Expansion Zerg Campaign Medium",
			(char*)(('x' << 24) | ('I' << 16) | ('H' << 8) | ('Z')), "Expansion Zerg Campaign Difficult",
			(char*)(('x' << 24) | ('U' << 16) | ('S' << 8) | ('Z')), "Expansion Zerg Campaign Insane",
			(char*)(('x' << 24) | ('R' << 16) | ('A' << 8) | ('Z')), "Expansion Zerg Campaign Area Town",
			(char*)(('x' << 24) | ('O' << 16) | ('L' << 8) | ('P')), "Expansion Protoss Campaign Easy",
			(char*)(('x' << 24) | ('E' << 16) | ('M' << 8) | ('P')), "Expansion Protoss Campaign Medium",
			(char*)(('x' << 24) | ('I' << 16) | ('H' << 8) | ('P')), "Expansion Protoss Campaign Difficult",
			(char*)(('x' << 24) | ('U' << 16) | ('S' << 8) | ('P')), "Expansion Protoss Campaign Insane",
			(char*)(('x' << 24) | ('R' << 16) | ('A' << 8) | ('P')), "Expansion Protoss Campaign Area Town",
			(char*)(('c' << 24) | ('i' << 16) | ('u' << 8) | ('S')), "Send All Units on Strategic Suicide Missions",
			(char*)(('R' << 24) | ('i' << 16) | ('u' << 8) | ('S')), "Send All Units on Random Suicide Missions",
			(char*)(('u' << 24) | ('c' << 16) | ('s' << 8) | ('R')), "Switch Computer Player to Rescue Passive",
			(char*)(('0' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 1",
			(char*)(('1' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 2",
			(char*)(('2' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 3",
			(char*)(('3' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 4",
			(char*)(('4' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 5",
			(char*)(('5' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 6",
			(char*)(('6' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 7",
			(char*)(('7' << 24) | ('i' << 16) | ('V' << 8) | ('+')), "Turn ON Shared Vision for Player 8",
			(char*)(('0' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 1",
			(char*)(('1' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 2",
			(char*)(('2' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 3",
			(char*)(('3' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 4",
			(char*)(('4' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 5",
			(char*)(('5' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 6",
			(char*)(('6' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 7",
			(char*)(('7' << 24) | ('i' << 16) | ('V' << 8) | ('-')), "Turn OFF Shared Vision for Player 8",
			(char*)(('e' << 24) | ('T' << 16) | ('v' << 8) | ('M')), "Move Dark Templars to Region",
			(char*)(('C' << 24) | ('r' << 16) | ('l' << 8) | ('C')), "Clear Previous Combat Data",
			(char*)(('y' << 24) | ('m' << 16) | ('n' << 8) | ('E')), "Set Player to Enemy",
			(char*)(('A' << 24) | ('l' << 16) | ('l' << 8) | ('y')), "Set Player to Ally",
			(char*)(('A' << 24) | ('u' << 16) | ('l' << 8) | ('V')), "Value This Area Higher",
			(char*)(('k' << 24) | ('B' << 16) | ('n' << 8) | ('E')), "Enter Closest Bunker",
			(char*)(('g' << 24) | ('T' << 16) | ('t' << 8) | ('S')), "Set Generic Command Target",
			(char*)(('t' << 24) | ('P' << 16) | ('t' << 8) | ('S')), "Make These Units Patrol",
			(char*)(('r' << 24) | ('T' << 16) | ('n' << 8) | ('E')), "Enter Transport",
			(char*)(('r' << 24) | ('T' << 16) | ('x' << 8) | ('E')), "Exit Transport",
			(char*)(('e' << 24) | ('H' << 16) | ('u' << 8) | ('N')), "AI Nuke Here",
			(char*)(('e' << 24) | ('H' << 16) | ('a' << 8) | ('H')), "AI Harass Here",
			(char*)(('g' << 24) | ('D' << 16) | ('Y' << 8) | ('J')), "Set Unit Order To: Junk Yard Dog",
			(char*)(('e' << 24) | ('H' << 16) | ('W' << 8) | ('D')), "Disruption Web Here",
			(char*)(('e' << 24) | ('H' << 16) | ('e' << 8) | ('R')), "Recall Here",
			(char*)(('3' << 24) | ('r' << 16) | ('e' << 8) | ('T')), "Terran 3 - Zerg Town",
			(char*)(('5' << 24) | ('r' << 16) | ('e' << 8) | ('T')), "Terran 5 - Terran Main Town",
			(char*)(('H' << 24) | ('5' << 16) | ('e' << 8) | ('T')), "Terran 5 - Terran Harvest Town",
			(char*)(('6' << 24) | ('r' << 16) | ('e' << 8) | ('T')), "Terran 6 - Air Attack Zerg",
			(char*)(('b' << 24) | ('6' << 16) | ('e' << 8) | ('T')), "Terran 6 - Ground Attack Zerg",
			(char*)(('c' << 24) | ('6' << 16) | ('e' << 8) | ('T')), "Terran 6 - Zerg Support Town",
			(char*)(('7' << 24) | ('r' << 16) | ('e' << 8) | ('T')), "Terran 7 - Bottom Zerg Town",
			(char*)(('s' << 24) | ('7' << 16) | ('e' << 8) | ('T')), "Terran 7 - Right Zerg Town",
			(char*)(('m' << 24) | ('7' << 16) | ('e' << 8) | ('T')), "Terran 7 - Middle Zerg Town",
			(char*)(('8' << 24) | ('r' << 16) | ('e' << 8) | ('T')), "Terran 8 - Confederate Town",
			(char*)(('L' << 24) | ('9' << 16) | ('r' << 8) | ('T')), "Terran 9 - Light Attack",
			(char*)(('H' << 24) | ('9' << 16) | ('r' << 8) | ('T')), "Terran 9 - Heavy Attack",
			(char*)(('0' << 24) | ('1' << 16) | ('e' << 8) | ('T')), "Terran 10 - Confederate Towns",
			(char*)(('z' << 24) | ('1' << 16) | ('1' << 8) | ('T')), "Terran 11 - Zerg Town",
			(char*)(('a' << 24) | ('1' << 16) | ('1' << 8) | ('T')), "Terran 11 - Lower Protoss Town",
			(char*)(('b' << 24) | ('1' << 16) | ('1' << 8) | ('T')), "Terran 11 - Upper Protoss Town",
			(char*)(('N' << 24) | ('2' << 16) | ('1' << 8) | ('T')), "Terran 12 - Nuke Town",
			(char*)(('P' << 24) | ('2' << 16) | ('1' << 8) | ('T')), "Terran 12 - Phoenix Town",
			(char*)(('T' << 24) | ('2' << 16) | ('1' << 8) | ('T')), "Terran 12 - Tank Town",
			(char*)(('1' << 24) | ('D' << 16) | ('E' << 8) | ('T')), "Terran 1 - Electronic Distribution",
			(char*)(('2' << 24) | ('D' << 16) | ('E' << 8) | ('T')), "Terran 2 - Electronic Distribution",
			(char*)(('3' << 24) | ('D' << 16) | ('E' << 8) | ('T')), "Terran 3 - Electronic Distribution",
			(char*)(('1' << 24) | ('W' << 16) | ('S' << 8) | ('T')), "Terran 1 - Shareware",
			(char*)(('2' << 24) | ('W' << 16) | ('S' << 8) | ('T')), "Terran 2 - Shareware",
			(char*)(('3' << 24) | ('W' << 16) | ('S' << 8) | ('T')), "Terran 3 - Shareware",
			(char*)(('4' << 24) | ('W' << 16) | ('S' << 8) | ('T')), "Terran 4 - Shareware",
			(char*)(('5' << 24) | ('W' << 16) | ('S' << 8) | ('T')), "Terran 5 - Shareware",
			(char*)(('1' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 1 - Terran Town",
			(char*)(('2' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 2 - Protoss Town",
			(char*)(('3' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 3 - Terran Town",
			(char*)(('4' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 4 - Right Terran Town",
			(char*)(('S' << 24) | ('4' << 16) | ('e' << 8) | ('Z')), "Zerg 4 - Lower Terran Town",
			(char*)(('6' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 6 - Protoss Town",
			(char*)(('a' << 24) | ('7' << 16) | ('r' << 8) | ('Z')), "Zerg 7 - Air Town",
			(char*)(('g' << 24) | ('7' << 16) | ('r' << 8) | ('Z')), "Zerg 7 - Ground Town",
			(char*)(('s' << 24) | ('7' << 16) | ('r' << 8) | ('Z')), "Zerg 7 - Support Town",
			(char*)(('8' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 8 - Scout Town",
			(char*)(('T' << 24) | ('8' << 16) | ('e' << 8) | ('Z')), "Zerg 8 - Templar Town",
			(char*)(('9' << 24) | ('r' << 16) | ('e' << 8) | ('Z')), "Zerg 9 - Teal Protoss",
			(char*)(('y' << 24) | ('l' << 16) | ('9' << 8) | ('Z')), "Zerg 9 - Left Yellow Protoss",
			(char*)(('y' << 24) | ('r' << 16) | ('9' << 8) | ('Z')), "Zerg 9 - Right Yellow Protoss",
			(char*)(('o' << 24) | ('l' << 16) | ('9' << 8) | ('Z')), "Zerg 9 - Left Orange Protoss",
			(char*)(('o' << 24) | ('r' << 16) | ('9' << 8) | ('Z')), "Zerg 9 - Right Orange Protoss",
			(char*)(('a' << 24) | ('0' << 16) | ('1' << 8) | ('Z')), "Zerg 10 - Left Teal (Attack",
			(char*)(('b' << 24) | ('0' << 16) | ('1' << 8) | ('Z')), "Zerg 10 - Right Teal (Support",
			(char*)(('c' << 24) | ('0' << 16) | ('1' << 8) | ('Z')), "Zerg 10 - Left Yellow (Support",
			(char*)(('d' << 24) | ('0' << 16) | ('1' << 8) | ('Z')), "Zerg 10 - Right Yellow (Attack",
			(char*)(('e' << 24) | ('0' << 16) | ('1' << 8) | ('Z')), "Zerg 10 - Red Protoss",
			(char*)(('1' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 1 - Zerg Town",
			(char*)(('2' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 2 - Zerg Town",
			(char*)(('R' << 24) | ('3' << 16) | ('r' << 8) | ('P')), "Protoss 3 - Air Zerg Town",
			(char*)(('G' << 24) | ('3' << 16) | ('r' << 8) | ('P')), "Protoss 3 - Ground Zerg Town",
			(char*)(('4' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 4 - Zerg Town",
			(char*)(('I' << 24) | ('5' << 16) | ('r' << 8) | ('P')), "Protoss 5 - Zerg Town Island",
			(char*)(('B' << 24) | ('5' << 16) | ('r' << 8) | ('P')), "Protoss 5 - Zerg Town Base",
			(char*)(('7' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 7 - Left Protoss Town",
			(char*)(('B' << 24) | ('7' << 16) | ('r' << 8) | ('P')), "Protoss 7 - Right Protoss Town",
			(char*)(('S' << 24) | ('7' << 16) | ('r' << 8) | ('P')), "Protoss 7 - Shrine Protoss",
			(char*)(('8' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 8 - Left Protoss Town",
			(char*)(('B' << 24) | ('8' << 16) | ('r' << 8) | ('P')), "Protoss 8 - Right Protoss Town",
			(char*)(('D' << 24) | ('8' << 16) | ('r' << 8) | ('P')), "Protoss 8 - Protoss Defenders",
			(char*)(('9' << 24) | ('o' << 16) | ('r' << 8) | ('P')), "Protoss 9 - Ground Zerg",
			(char*)(('W' << 24) | ('9' << 16) | ('r' << 8) | ('P')), "Protoss 9 - Air Zerg",
			(char*)(('Y' << 24) | ('9' << 16) | ('r' << 8) | ('P')), "Protoss 9 - Spell Zerg",
			(char*)(('0' << 24) | ('1' << 16) | ('r' << 8) | ('P')), "Protoss 10 - Mini-Towns",
			(char*)(('C' << 24) | ('0' << 16) | ('1' << 8) | ('P')), "Protoss 10 - Mini-Town Master",
			(char*)(('o' << 24) | ('0' << 16) | ('1' << 8) | ('P')), "Protoss 10 - Overmind Defenders",
			(char*)(('A' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town A",
			(char*)(('B' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town B",
			(char*)(('C' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town C",
			(char*)(('D' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town D",
			(char*)(('E' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town E",
			(char*)(('F' << 24) | ('1' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 1 - Town F",
			(char*)(('A' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town A",
			(char*)(('B' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town B",
			(char*)(('C' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town C",
			(char*)(('D' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town D",
			(char*)(('E' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town E",
			(char*)(('F' << 24) | ('2' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 2 - Town F",
			(char*)(('A' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town A",
			(char*)(('B' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town B",
			(char*)(('C' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town C",
			(char*)(('D' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town D",
			(char*)(('E' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town E",
			(char*)(('F' << 24) | ('3' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 3 - Town F",
			(char*)(('A' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town A",
			(char*)(('B' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town B",
			(char*)(('C' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town C",
			(char*)(('D' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town D",
			(char*)(('E' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town E",
			(char*)(('F' << 24) | ('4' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 4 - Town F",
			(char*)(('A' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town A",
			(char*)(('B' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town B",
			(char*)(('C' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town C",
			(char*)(('D' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town D",
			(char*)(('E' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town E",
			(char*)(('F' << 24) | ('5' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 5 - Town F",
			(char*)(('A' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town A",
			(char*)(('B' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town B",
			(char*)(('C' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town C",
			(char*)(('D' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town D",
			(char*)(('E' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town E",
			(char*)(('F' << 24) | ('6' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 6 - Town F",
			(char*)(('A' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town A",
			(char*)(('B' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town B",
			(char*)(('C' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town C",
			(char*)(('D' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town D",
			(char*)(('E' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town E",
			(char*)(('F' << 24) | ('7' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 7 - Town F",
			(char*)(('A' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town A",
			(char*)(('B' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town B",
			(char*)(('C' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town C",
			(char*)(('D' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town D",
			(char*)(('E' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town E",
			(char*)(('F' << 24) | ('8' << 16) | ('B' << 8) | ('P')), "Brood Wars Protoss 8 - Town F",
			(char*)(('A' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town A",
			(char*)(('B' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town B",
			(char*)(('C' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town C",
			(char*)(('D' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town D",
			(char*)(('E' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town E",
			(char*)(('F' << 24) | ('1' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 1 - Town F",
			(char*)(('A' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town A",
			(char*)(('B' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town B",
			(char*)(('C' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town C",
			(char*)(('D' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town D",
			(char*)(('E' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town E",
			(char*)(('F' << 24) | ('2' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 2 - Town F",
			(char*)(('A' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town A",
			(char*)(('B' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town B",
			(char*)(('C' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town C",
			(char*)(('D' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town D",
			(char*)(('E' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town E",
			(char*)(('F' << 24) | ('3' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 3 - Town F",
			(char*)(('A' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town A",
			(char*)(('B' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town B",
			(char*)(('C' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town C",
			(char*)(('D' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town D",
			(char*)(('E' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town E",
			(char*)(('F' << 24) | ('4' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 4 - Town F",
			(char*)(('A' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town A",
			(char*)(('B' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town B",
			(char*)(('C' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town C",
			(char*)(('D' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town D",
			(char*)(('E' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town E",
			(char*)(('F' << 24) | ('5' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 5 - Town F",
			(char*)(('A' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town A",
			(char*)(('B' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town B",
			(char*)(('C' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town C",
			(char*)(('D' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town D",
			(char*)(('E' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town E",
			(char*)(('F' << 24) | ('6' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 6 - Town F",
			(char*)(('A' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town A",
			(char*)(('B' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town B",
			(char*)(('C' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town C",
			(char*)(('D' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town D",
			(char*)(('E' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town E",
			(char*)(('F' << 24) | ('7' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 7 - Town F",
			(char*)(('A' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town A",
			(char*)(('B' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town B",
			(char*)(('C' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town C",
			(char*)(('D' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town D",
			(char*)(('E' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town E",
			(char*)(('F' << 24) | ('8' << 16) | ('B' << 8) | ('T')), "Brood Wars Terran 8 - Town F",
			(char*)(('A' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town A",
			(char*)(('B' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town B",
			(char*)(('C' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town C",
			(char*)(('D' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town D",
			(char*)(('E' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town E",
			(char*)(('F' << 24) | ('1' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 1 - Town F",
			(char*)(('A' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town A",
			(char*)(('B' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town B",
			(char*)(('C' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town C",
			(char*)(('D' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town D",
			(char*)(('E' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town E",
			(char*)(('F' << 24) | ('2' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 2 - Town F",
			(char*)(('A' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town A",
			(char*)(('B' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town B",
			(char*)(('C' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town C",
			(char*)(('D' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town D",
			(char*)(('E' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town E",
			(char*)(('F' << 24) | ('3' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 3 - Town F",
			(char*)(('A' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town A",
			(char*)(('B' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town B",
			(char*)(('C' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town C",
			(char*)(('D' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town D",
			(char*)(('E' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town E",
			(char*)(('F' << 24) | ('4' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 4 - Town F",
			(char*)(('A' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town A",
			(char*)(('B' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town B",
			(char*)(('C' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town C",
			(char*)(('D' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town D",
			(char*)(('E' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town E",
			(char*)(('F' << 24) | ('5' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 5 - Town F",
			(char*)(('A' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town A",
			(char*)(('B' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town B",
			(char*)(('C' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town C",
			(char*)(('D' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town D",
			(char*)(('E' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town E",
			(char*)(('F' << 24) | ('6' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 6 - Town F",
			(char*)(('A' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town A",
			(char*)(('B' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town B",
			(char*)(('C' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town C",
			(char*)(('D' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town D",
			(char*)(('E' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town E",
			(char*)(('F' << 24) | ('7' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 7 - Town F",
			(char*)(('A' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town A",
			(char*)(('B' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town B",
			(char*)(('C' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town C",
			(char*)(('D' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town D",
			(char*)(('E' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town E",
			(char*)(('F' << 24) | ('8' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 8 - Town F",
			(char*)(('A' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town A",
			(char*)(('B' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town B",
			(char*)(('C' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town C",
			(char*)(('D' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town D",
			(char*)(('E' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town E",
			(char*)(('F' << 24) | ('9' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 9 - Town F",
			(char*)(('A' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town A",
			(char*)(('B' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town B",
			(char*)(('C' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town C",
			(char*)(('D' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town D",
			(char*)(('E' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town E",
			(char*)(('F' << 24) | ('0' << 16) | ('B' << 8) | ('Z')), "Brood Wars Zerg 10 - Town F",
		};
		PRINT_R("TRIGGERS", "\"");
		this->printAssoc(wb, error, 293, (char**)values);
		PRINT_R("TRIGGERS", "\"");
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
			char* str = STR->getRawString(this->value);
			PRINT_R("TRIGGERS", "Location %d \"%s\"", this->value, str);
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
