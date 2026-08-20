#include <util/input/BytesDataInput.hpp>
#include <stdint.h>
#include <string.h>

#if defined(__PS3__) || defined(__PPU__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BSWAP16(x) __builtin_bswap16(x)
#define BSWAP32(x) __builtin_bswap32(x)
#define BSWAP64(x) __builtin_bswap64(x)
#else
#define BSWAP16(x) (x)
#define BSWAP32(x) (x)
#define BSWAP64(x) (x)
#endif

BytesDataInput::~BytesDataInput(){
}

int8_t BytesDataInput::readByte() {
	int8_t v4 = 0;
	this->readBytes(&v4, 1);
	return v4;
}

double BytesDataInput::readDouble() {
	union { double d; uint64_t u; } val;
	val.u = 0;
	if (this->readBytes(&val.u, 8)) {
		val.u = BSWAP64(val.u);
	}
	return val.d;
}

float BytesDataInput::readFloat() {
	union { float f; uint32_t u; } val;
	val.u = 0;
	if (this->readBytes(&val.u, 4)) {
		val.u = BSWAP32(val.u);
	}
	return val.f;
}

int32_t BytesDataInput::readInt() {
	int32_t v4 = 0;
	if (this->readBytes(&v4, 4)) {
		v4 = (int32_t)BSWAP32((uint32_t)v4);
	}
	return v4;
}

int64_t BytesDataInput::readLongLong() {
	int64_t v4 = 0;
	if (this->readBytes(&v4, 8)) {
		v4 = (int64_t)BSWAP64((uint64_t)v4);
	}
	return v4;
}

int16_t BytesDataInput::readShort() {
	int16_t v4 = 0;
	if (this->readBytes(&v4, 2)) {
		v4 = (int16_t)BSWAP16((uint16_t)v4);
	}
	return v4;
}

std::string BytesDataInput::readString() {
	int16_t slen = this->readShort();
	int v6 = 0;

	if(slen > 0) {
		if(slen >= 0x7fff) v6 = 32766;
		else v6 = slen;

		char* v7 = new char[v6 + 1];
		this->readBytes(v7, v6);
		v7[v6] = 0;
		std::string ret(v7, v6);
		delete[] v7;
		return ret;
	}

	return "";
}
