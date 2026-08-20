#include <network/mco/MCOPayloadPacker.hpp>
#include <util/Random.hpp>
#include <BitStream.h>
#include <stdio.h> // Añadido para el printf

MCOPayloadPacker::MCOPayloadPacker(Random& a2) {
	this->random = &a2;
}

// ARREGLADO: Quitamos el punto y coma y le damos un cuerpo que devuelve un texto vacio
std::string MCOPayloadPacker::readControlPackage(char_t*, uint32_t) {
	printf("MCOPayloadPacker::readControlPackage - not implemented\n");
	return "";
}

std::string MCOPayloadPacker::writeBitStream(int64_t a3, std::string a4) {
	unsigned int v14 = this->random->genrand_int32();
	unsigned int v15 = this->random->genrand_int32();
	unsigned int v16 = this->random->genrand_int32();
	unsigned int v17 = this->random->genrand_int32();
	RakNet::BitStream v19;
	short v13 = a4.size();
	v19.Write<long long>(a3); //inlined
	v19.Write<short>(v13);
	v19.Write(a4.c_str(), a4.size());
	v19.Write<unsigned int>(v14);
	v19.Write<unsigned int>(v15);
	v19.Write<unsigned int>(v16);
	v19.Write<unsigned int>(v17);
	return std::string((const char*) v19.GetData(), v19.GetNumberOfBytesUsed());
}

// ARREGLADO: Quitamos el punto y coma y le damos un cuerpo
std::string MCOPayloadPacker::writeControllPackage(const ControllerData&) {
	printf("MCOPayloadPacker::writeControllPackage - not implemented\n");
	return "";
}
