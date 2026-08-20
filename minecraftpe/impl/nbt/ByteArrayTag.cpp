#include <nbt/ByteArrayTag.hpp>
#include <util/input/IDataInput.hpp>
#include <util/output/IDataOutput.hpp>
#include <string.h>
#include <stdio.h>
#include <string>

ByteArrayTag::ByteArrayTag(const std::string& n, int8_t* arr, int32_t length) : Tag(n){
	this->value = arr;
	this->count = length;
}

void ByteArrayTag::write(IDataOutput* out){
	out->writeInt(this->count);
	if (this->count > 0 && this->value) {
		out->writeBytes(this->value, this->count);
	}
}

void ByteArrayTag::load(IDataInput* in){
	int32_t n = in->readInt();

	// BLINDAJE: Si 'n' es negativo o absurdamente grande (ej. -1 por desincronización de bytes)
	if (n <= 0 || n > 16 * 1024 * 1024) {
		if (n < 0) {
			printf("[ByteArrayTag] BLOQUEADO intento de asignacion negativa (%d bytes).\n", n);
			fflush(stdout);
		}
		this->value = nullptr;
		this->count = 0;
		return;
	}

	int8_t* arr = new (std::nothrow) int8_t[n];
	if (!arr) {
		printf("[ByteArrayTag] ERROR: Sin RAM para %d bytes.\n", n);
		fflush(stdout);
		this->value = nullptr;
		this->count = 0;
		return;
	}

	this->value = arr;
	this->count = n; // ¡Arreglamos el bug original donde no actualizaba el count!
	in->readBytes(this->value, n);
}

int32_t ByteArrayTag::getId(void) const{
	return 7;
}

std::string ByteArrayTag::toString(){
	char buf[64];
	snprintf(buf, sizeof(buf), "[%d bytes]", (int)this->count);
	return std::string(buf);
}

Tag* ByteArrayTag::copy(void){
	if (this->count <= 0 || !this->value) {
		return new ByteArrayTag(this->getName(), nullptr, 0);
	}
	int8_t* arr = new int8_t[this->count];
	memcpy(arr, this->value, this->count);
	return new ByteArrayTag(this->getName(), arr, this->count);
}

bool_t ByteArrayTag::equals(const Tag& t){
	const ByteArrayTag* tg = (const ByteArrayTag*) &t;
	bool_t eq = Tag::equals(t);

	if(eq){
		int32_t count = this->count;
		if (count != tg->count) return 0;
		if (count == 0) return 1;
		if (!this->value || !tg->value) return this->value == tg->value;
		return memcmp(this->value, tg->value, count) == 0;
	}
	return eq;
}
