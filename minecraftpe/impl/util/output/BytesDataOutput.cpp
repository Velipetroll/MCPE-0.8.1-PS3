#include <util/output/BytesDataOutput.hpp>
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

BytesDataOutput::~BytesDataOutput() {
}

void BytesDataOutput::writeString(const std::string& a2) {
	this->writeShort(a2.size() & 0x7fff);
	this->writeBytes(a2.data(), a2.size() & 0x7fff);
}

void BytesDataOutput::writeFloat(float a2) {
	union { float f; uint32_t u; } val;
	val.f = a2;
	uint32_t swapped = BSWAP32(val.u);
	this->writeBytes(&swapped, 4);
}

void BytesDataOutput::writeDouble(double a2) {
	union { double d; uint64_t u; } val;
	val.d = a2;
	uint64_t swapped = BSWAP64(val.u);
	this->writeBytes(&swapped, 8);
}

void BytesDataOutput::writeByte(int8_t a2) {
	this->writeBytes(&a2, 1);
}

void BytesDataOutput::writeShort(int16_t a2) {
	uint16_t swapped = BSWAP16((uint16_t)a2);
	this->writeBytes(&swapped, 2);
}

void BytesDataOutput::writeInt(int32_t a2) {
	uint32_t swapped = BSWAP32((uint32_t)a2);
	this->writeBytes(&swapped, 4);
}

void BytesDataOutput::writeLongLong(int64_t a2) {
	uint64_t swapped = BSWAP64((uint64_t)a2);
	this->writeBytes(&swapped, 8);
}
