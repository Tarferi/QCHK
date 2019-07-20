#include "CHK.h"
#include "common.h"

CHK::CHK(SoundCollection* sounds, FILE * file)
{
	this->closeFileAfter = false;
	bool error;
	this->buffer = new ReadBuffer(file, &error);
	if (error) {

	}
	this->sounds = sounds;
	
	this->valid = this->parse();
}

CHK::CHK(SoundCollection* sounds, const char * fileName)
{
	this->closeFileAfter = false;
	errno_t err = fopen_s(&(this->file), fileName, "rb");
	if (!this->file || err) {
		this->valid = false;
		return;
	}
	this->closeFileAfter = true;
	bool error = false;
	this->buffer = new ReadBuffer(this->file, &error);
	if (error) {
		this->valid = false;
		return;
	}
	this->sounds = sounds;
	this->valid = this->parse();
}

CHK::CHK(SoundCollection* sounds, char * data, unsigned int size)
{
	this->closeFileAfter = false;
	bool error = false;
	this->buffer = new ReadBuffer((unsigned char*) data, size, &error);
	if (error) {
		this->valid = false;
		return;
	}
	this->sounds = sounds;
	this->valid = this->parse();
}

CHK::~CHK()
{
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		if (section != nullptr) {
			if (!strcmp(section->getName(), "TRIG")) {
				delete section;
				continue;
			}
			delete section;
		}
		section = nullptr;
	};
	delete this->buffer;
	if (this->closeFileAfter) {
		fclose(this->file);
		this->closeFileAfter = false;
	}
}

bool CHK::write(WriteBuffer* buffer) {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		bool error = false;
		buffer->writeFixedLengthString((unsigned char*) section->getName(), &error);
		if (error) {
			LOG("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		unsigned int prePosition = buffer->getPosition();
		buffer->writeInt(0, &error); // Later replace this with size
		if (error) {
			LOG("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		section->write(buffer);
		unsigned int postPosition = buffer->getPosition();
		unsigned int sectionSize = (postPosition - prePosition) - 4;
		buffer->setPosition(prePosition);
		buffer->writeInt(sectionSize, &error);
		if (error) {
			LOG("CHK", "Failed to write section \"%s\"", section->getName());
			return false;
		}
		buffer->setPosition(postPosition);
	}
	return true;
}

bool CHK::write(const char * file) {
	FILE* f;
	if (fopen_s(&f, file, "wb")) {
		LOG("CHK", "Failed to open file \"%s\" for writing", file);
		return false;
	}
	WriteBuffer* wb = new WriteBuffer();
	if (!this->write(wb)) {
		fclose(f);
		return false;
	}
	unsigned int size;
	unsigned char* data;
	wb->getWrittenData(&data, &size);
	unsigned int toWrite = size;
	while (toWrite != 0) {
		int written = fwrite(data, sizeof(unsigned char), toWrite, f);
		toWrite -= written;
		data = &(data[written]);
	}
	fclose(f);
	delete wb;
	return true;
}


Section * CHK::getSection(const char * name) {
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		char* sname = section->getName();
		if (!strcmp(name, sname)) {
			return section;
		}
	};
	return nullptr;
}

void CHK::removeSection(Section* section)
{

	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* sec = this->sections[i];
		if (sec == section) {
			this->sections.remove(i);
			return;
		}
	}
}

void CHK::setSection(Section * newSection, bool* error)
{
	for (unsigned int i = 0; i < this->sections.getSize(); i++) {
		Section* section = this->sections[i];
		char* sname = section->getName();
		if (!strcmp(newSection->getName(), sname)) {
			this->sections[i] = newSection;
			delete section;
			return;
		}
	};
	if (!this->sections.append(newSection)) {
		delete newSection; // This section was meant to be managed (and deleted) by this class, delete it now
		*error = true;
	}
}

bool contains(Array<char*>* array, char* value) {
	for (unsigned int itemIndex = 0; itemIndex < array->getSize(); itemIndex++) {
		if (!strcmp(array->get(itemIndex), value)) {
			return true;
		}
	}
	return false;
}

bool contains(Array<unsigned short>* indexes, unsigned short index) {
	for (unsigned int indexIndex = 0; indexIndex < indexes->getSize(); indexIndex++) {
		unsigned short aIndex = indexes->get(indexIndex);
		if (aIndex == index) {
			return true;
		}
	}
	return false;
}

Array<unsigned short>* CHK::getUsedWavs(Array<Action*>* actionSetter, bool* error) {
	//Array<char*>* array = new Array<char*>();
	Array<unsigned short>* indexes = new Array<unsigned short>();
	Section_TRIG* trig = (Section_TRIG*) this->getSection("TRIG");
	Section_TRIG* mbrf = (Section_TRIG*) this->getSection("MBRF");
	Section_STR_* str = (Section_STR_*) this->getSection("STR ");
	for (unsigned int type = 0; type < 2; type++) {
		Section_TRIG* SRC = type == 0 ? trig : mbrf;
		for (unsigned int triggerIndex = 0; triggerIndex < SRC->triggers.getSize(); triggerIndex++) {
			Trigger* trigger = SRC->triggers[triggerIndex];
			bool enabled = false;
			for (unsigned int conditionIndex = 0; conditionIndex < 16; conditionIndex++) {
				Condition* condition = &(trigger->conditions[conditionIndex]);
				if ((condition->Flags & 2) == 0) { // Enabled
					if (condition->ConditionType > 0 && condition->ConditionType != 23) { // Not never
						enabled = true;
						break;
					}
				}
			}
			if(enabled){
				for (unsigned int actionIndex = 0; actionIndex < 64; actionIndex++) {
					Action* action = &(trigger->actions[actionIndex]);
					if (action->ActionType == 7 || action->ActionType == 8) { // Play wavs
						if (actionSetter != nullptr) {
							if (!actionSetter->append(action)){
								*error = true;
								return 0;
							}
						}
						unsigned short index = (unsigned short)action->WAVStringNumber;
						 if (!contains(indexes, index)) {
							if (!indexes->append(index)){
								*error = true;
								return 0;
							}
						}
					}
				}
			}
		}
	}
	//delete array;
	return indexes;
}

bool CHK::parse() {
	LOG("CHK", "BEGIN PARSING of %d bytes", this->buffer->getDataSize())
	while (!this->buffer->isDone()) {
		bool error = false;
		char* name =(char*) this->buffer->readFixedLengthString(4, &error);
		if (error) {
			LOG("CHK", "Error reading section name");
			return false;
		}
		unsigned int size = this->buffer->readInt(&error);
		if (error) {
			LOG("CHK", "Error reading section size");
			return false;
		}
		LOG("CHK", "Found section \"%s\" of size %d", name, size);
		Section* section;
		if (!strcmp(name, "TYPE")) {
			section = new Section_TYPE((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "VER ")) {
			section = new Section_VER_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "IVER")) {
			section = new Section_IVER((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "IVE2")) {
			section = new Section_IVE2((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "VCOD")) {
			section = new Section_VCOD((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "IOWN")) {
			section = new Section_IOWN((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "OWNR")) {
			section = new Section_OWNR((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "ERA ")) {
			section = new Section_ERA_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "DIM ")) {
			section = new Section_DIM_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "SIDE")) {
			section = new Section_SIDE((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "MTXM")) {
			section = new Section_MTXM((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "PUNI")) {
			section = new Section_PUNI((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UPGR")) {
			section = new Section_UPGR((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "PTEC")) {
			section = new Section_PTEC((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UNIT")) {
			section = new Section_UNIT((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "ISOM")) {
			section = new Section_ISOM((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "TILE")) {
			section = new Section_TILE((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "DD2 ")) {
			section = new Section_DD2_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "THG2")) {
			section = new Section_THG2((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "MASK")) {
			section = new Section_MASK((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "STR ")) {
			section = new Section_STR_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UPRP")) {
			section = new Section_UPRP((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UPUS")) {
			section = new Section_UPUS((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "MRGN")) {
			section = new Section_MRGN((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "TRIG")) {
			section = new Section_TRIG((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "MBRF")) {
			section = new Section_MBRF((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "SPRP")) {
			section = new Section_SPRP((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "FORC")) {
			section = new Section_FORC((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "WAV ")) {
			section = new Section_WAV_((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UNIS")) {
			section = new Section_UNIS((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UPGS")) {
			section = new Section_UPGS((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "TECS")) {
			section = new Section_TECS((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "SWNM")) {
			section = new Section_SWNM((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "COLR")) {
			section = new Section_COLR((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "PUPx")) {
			section = new Section_PUPx((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "PTEx")) {
			section = new Section_PTEx((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UNIx")) {
			section = new Section_UNIx((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "UPGx")) {
			section = new Section_UPGx((unsigned char*)name, size, this->buffer);
		}
		else if (!strcmp(name, "TECx")) {
			section = new Section_TECx((unsigned char*)name, size, this->buffer);
		}
		else {
			section = new BasicSection((unsigned char*)name, size, this->buffer);
		}
		if (!this->sections.append(section)) {
			delete section;
			return false;
		}
		if (!section->process()) {
			LOG("CHK", "Failed to process section \"%s\" (size %d)", name, size);
			return false;
		}
	}
	return true;
}

