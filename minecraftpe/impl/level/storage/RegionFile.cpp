#include <level/storage/RegionFile.hpp>
#include <BitStream.h>
#include <string.h>
#include <stdio.h>

#if defined(__PS3__) || defined(__PPU__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BSWAP32(x) __builtin_bswap32(x)
#else
#define BSWAP32(x) (x)
#endif

RegionFile::RegionFile(const std::string& a2) {
	this->fileRaw = 0;
	this->path2file = a2;
	this->path2file += "/";
	this->path2file += "chunks.dat";
	this->locTable = new int32_t[1024];
	this->bytes4096_2 = new int8_t[4096];
	memset(this->bytes4096_2, 0, 4096);
}

void RegionFile::close() {
	if(this->fileRaw) {
		fclose(this->fileRaw);
		this->fileRaw = 0;
	}
}

bool_t RegionFile::open() {
	this->close();
	memset(this->locTable, 0, 4096u);
	FILE* file = fopen(this->path2file.c_str(), "r+b");
	this->fileRaw = file;
	if(file) // Archivo existente
	{
		fread(this->locTable, 4u, 1024u, file);

		// CORRECCIÓN PS3: Invertimos los 1024 punteros de Chunks de Little a Big Endian
		for(int j = 0; j < 1024; ++j) {
			this->locTable[j] = BSWAP32(this->locTable[j]);
		}

		int32_t v3 = 0;
		int32_t v9 = 0;
		this->stdMap[v9] = 0;
		do {
			int32_t v4 = this->locTable[v3];
			if(v4) {
				int32_t v5 = v4 >> 8;
				int32_t v6 = (uint8_t)v4;
				for(int32_t i = 0; i < v6; ++i) {
					v9 = i + v5;
					this->stdMap[v9] = 0;
				}
			}
			++v3;
		} while(v3 != 1024);
	} else { // Archivo nuevo
		FILE* result = fopen(this->path2file.c_str(), "w+b");
		this->fileRaw = result;
		if(!result) {
			return 0;
		}
		fwrite(this->locTable, 4u, 1024u, result);
		int32_t v9 = 0;
		this->stdMap[v9] = 0;
	}
	return this->fileRaw != 0;
}

bool_t RegionFile::readChunk(int32_t chunkX, int32_t chunkZ, RakNet::BitStream** a4) {
	if (chunkX < 0 || chunkX >= 32 || chunkZ < 0 || chunkZ >= 32) return 0;
	if (!this->fileRaw) return 0;

	int32_t result = this->locTable[32 * chunkZ + chunkX];
	if(result) {
		int32_t sectorOffset = result >> 8;
		fseek(this->fileRaw, sectorOffset << 12, 0); // sector * 4096

		int32_t n = 0;
		if (fread(&n, 4u, 1u, this->fileRaw) != 1) return 0;
		n = BSWAP32(n); // Convertimos el tamaño del chunk

		// BLINDAJE CRÍTICO: Si 'n' es <= 4, el chunk está vacío o corrupto.
		if(n <= 4 || n > 1024 * 1024) {
			return 0;
		}

		n -= 4; // Restamos el header
		uint8_t* v8 = new (std::nothrow) uint8_t[n];
		if (!v8) return 0;

		int32_t bytesRead = fread(v8, 1u, n, this->fileRaw);
		if (bytesRead != n) {
			delete[] v8;
			return 0;
		}

		*a4 = new RakNet::BitStream(v8, n, 0);
		return 1;
	}
	return 0;
}

bool_t RegionFile::write(int32_t a2, RakNet::BitStream& a3) {
	if (!this->fileRaw) return 0;
	fseek(this->fileRaw, a2 << 12, 0);
	int32_t v6 = a3.GetNumberOfBytesUsed() + 4;
	int32_t v6_le = BSWAP32(v6); // Escribimos en Little-Endian para compatibilidad
	fwrite(&v6_le, 4u, 1u, this->fileRaw);
	fwrite(a3.GetData(), 1u, a3.GetNumberOfBytesUsed(), this->fileRaw);
	return 1;
}

bool_t RegionFile::writeChunk(int32_t chunkX, int32_t chunkZ, RakNet::BitStream& a4) {
	if (!this->fileRaw) return 0;
	int32_t regionIndex = chunkX + 32 * chunkZ;
	int32_t off = regionIndex;
	int32_t locTableEntry = this->locTable[regionIndex];
	int32_t v8 = ((int32_t)(a4.GetNumberOfBytesUsed() + 4) >> 12) + 1;

	if(v8 <= 256) {
		int32_t firstByteOfLocTableEntry = (uint8_t)locTableEntry;
		int32_t v10 = locTableEntry >> 8;
		int32_t v11 = 0;
		int32_t v13 = 1;
		int32_t v14 = 0;
		int32_t v16 = 0;
		int32_t v22 = 0;
		int32_t v24 = 0;
		int32_t v26 = 0;
		int32_t v30 = 0;
		int32_t entry_le = 0;

		if(v10) {
			if(firstByteOfLocTableEntry == v8) {
				this->write(v10, a4);
				return 1;
			}
		}

		while(v11 < firstByteOfLocTableEntry) {
			v30 = v11 + v10;
			v26 = v13;
			this->stdMap[v30] = v26;
			v13 = v26;
			++v11;
		}

		while(1) {
			int32_t v17 = v16 + v14;
			auto&& p = this->stdMap.find(v17);
			if(p == this->stdMap.end()) {
				break;
			}
			v30 = v16 + v14;
			if(this->stdMap[v30]) {
				if(++v14 >= v8) {
					goto LABEL_25;
				}
			} else {
				v16 += v14 + 1;
				v14 = 0;
			}
		}

		fseek(this->fileRaw, 0, 2);
		for(int32_t i = v8 - v14; v22 < i;) {
			int32_t v28 = i;
			fwrite(this->bytes4096_2, 4u, 0x400u, this->fileRaw);
			v30 = v22 + v16;
			++v22;
			this->stdMap[v30] = 1;
			i = v28;
		}

		LABEL_25:
		this->locTable[regionIndex] = v8 | (v16 << 8);
		do {
			v30 = v24 + v16;
			++v24;
			this->stdMap[v30] = 0;
		} while ( v24 < v8 );

		this->write(v16, a4);
		fseek(this->fileRaw, off * 4, 0);
		entry_le = BSWAP32(this->locTable[off]); // Guardamos en Little-Endian
		fwrite(&entry_le, 4u, 1u, this->fileRaw);
		return 1;
	}
	return 0;
}

RegionFile::~RegionFile() {
	this->close();
	if(this->locTable) delete[] this->locTable;
	if(this->bytes4096_2) delete[] this->bytes4096_2;
}
