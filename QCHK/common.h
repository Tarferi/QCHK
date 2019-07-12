#pragma once
#include "stdafx.h"
#include "stdlib.h"
#include <stdarg.h>
#include "string.h"

#define TRIG_PRINT

#define MALLOC(target, type, size, failBlock) target = (type*) malloc(sizeof(type)*(size)); if(!target){failBlock};

#define MALLOC_N(target, type, size, failBlock) type* target; MALLOC(target, type, size, failBlock);

#define COMMON_CONSTR_SEC(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : Section(name, size, buffer) {};

#define COMMON_CONSTR_SEC_BS(name) name(unsigned char* name, unsigned int size, ReadBuffer* buffer) : BasicSection(name, size, buffer) {};

#define ENDS_WIDTH(name, suffix) (strlen(name) >= strlen(suffix) ? !strcmp(&(name[strlen(name) - strlen(suffix)]), (char*) suffix): false)

#define GET_CLONED_DATA_SZ(target, type, string, length, spec_sz, failBlock) MALLOC_N(target, type, length + spec_sz, failBlock);if(target){ memcpy(target, string, length*sizeof(type));}

#define GET_CLONED_DATA(target, type, string, length, failBlock) GET_CLONED_DATA_SZ(target, type, string, length, 0, failBlock)

#define GET_CLONED_STRING_LEN(target, string, length, failBlock) GET_CLONED_DATA_SZ(target, char, string, length, 1, failBlock); target[length] = 0

#define GET_CLONED_STRING(target, string, failBlock) GET_CLONED_STRING_LEN(target, string, strlen(string), failBlock)

#define ARRAY_DEFAULT_SIZE 64
#define ARRAY_INCREATE_FACTOR 2;

static int initValue = ARRAY_DEFAULT_SIZE;

#ifdef _DEBUG

#define LOG_R(section, fmt, ...) \
	if(!strcmp(section, "TRIGGERS")) { do {fprintf(stderr, fmt, __VA_ARGS__); } while (0); } else {\
	do { fprintf(stderr, "[" section "] " fmt , __VA_ARGS__); } while (0);}

#define LOG(section, fmt, ...) \
		LOG_R(section, fmt "\n", __VA_ARGS__)

#else
	#define LOG(section, fmt, ...)
	#define LOG_R(section, fmt, ...)
#endif
template<typename type> class Array {

	class ArrayProxy {
		Array* array;
		int index;
	public:
		ArrayProxy(Array* array, int index) {
			this->array = array;
			this->index = index;
		}
		type operator= (type value) { array->set(index, value); return array->get(index); }
		operator type() { return array->get(index); }

	};

public:

	void remove(unsigned int index) {
		for (unsigned int i = index; i < this->dataSize - 1; i++) {
			this->rawData[i] = this->rawData[i + 1];
		}
		this->rawData[this->dataSize - 1] = (type) nullptr;
		this->dataSize--;
	}

	bool set(unsigned int index, type value) {
		bool error = false;
		if (index > this->size) {
			this->ensureAdditionalSize(this->size - index, &error);
		}
		if (error) {
			return false;
		}
		this->rawData[index] = value;
		if (index > this->dataSize) {
			this->dataSize = index + 1;
		}
		return true;
	}

	type get(unsigned int index) {
		return this->rawData[index];
	}

	unsigned int getSize() {
		return this->dataSize;
	}

	bool append(type value) {
		bool error = false;
		if (this->dataSize + sizeof(type) >= this->size) {
			this->ensureAdditionalSize(32 * sizeof(type), &error);
		}
		if (error) {
			return false;
		}
		this->rawData[this->dataSize] = value;
		this->dataSize++;
		return true;
	}

	void insert(unsigned int index, type value) {
		append(value);
		for (unsigned int i = this->dataSize - 1; i > index ; i--) {
			this->rawData[i] = this->rawData[i - 1];
		}
		this->rawData[index] = value;
	}

	void freeItems() {
		for (unsigned int i = 0; i < this->getSize(); i++) {
			char* fn = this->get(i);
			free(fn);
		}
	}

	ArrayProxy operator[] (unsigned int index) {
		return ArrayProxy(this, index);
	}

	~Array() {
		if (this->rawData != nullptr) {
			free(this->rawData);
			this->rawData = nullptr;
		}
	}


private:

	type* rawData = nullptr;

	unsigned int size = 0;

	unsigned  int dataSize = 0;

	void ensureAdditionalSize(unsigned int size, bool* error) {
		if (this->dataSize + size > this->size) {
			if (this->rawData != nullptr) {
				void* toFree = this->rawData;
				unsigned int newSize = this->size * ARRAY_INCREATE_FACTOR;
				MALLOC(this->rawData, type, newSize, { free(toFree); *error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				memcpy(this->rawData, toFree, this->size * sizeof(type));
				this->size = newSize;
				free(toFree);
			}
			else {
				unsigned int newSize = ARRAY_DEFAULT_SIZE;
				MALLOC(this->rawData, type, newSize, { *error = true; return; });
				memset(this->rawData, 0, newSize * sizeof(type));
				this->size = newSize;
				this->dataSize = 0;
			}
			this->ensureAdditionalSize(size, error);
		}
	}

};

#define LIBRARY_API __declspec(dllexport)
