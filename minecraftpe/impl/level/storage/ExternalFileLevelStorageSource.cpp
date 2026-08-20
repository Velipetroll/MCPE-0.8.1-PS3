#include <level/storage/ExternalFileLevelStorageSource.hpp>
#include <compression.hpp>
#include <cpputils.hpp>
#include <dirent.h>
#include <level/storage/ExternalFileLevelStorage.hpp>
#include <level/storage/LevelData.hpp>
#include <sstream>
#include <cstdio> // Para printf

ExternalFileLevelStorageSource::ExternalFileLevelStorageSource(const std::string& a2, const std::string& a3) {
	printf("[Storage] Iniciando ExternalFileLevelStorageSource...\n"); fflush(stdout);
	printf("[Storage] Rutas base: '%s' y '%s'\n", a2.c_str(), a3.c_str()); fflush(stdout);

	this->field_10 = !(a3 == a2);
	const char* v10[] = {"/games", "/com.mojang", "/minecraftWorlds"};

	printf("[Storage] Creando arbol de carpetas 1...\n"); fflush(stdout);
	createTree(a2.c_str(), v10, 3);

	if(this->field_10) {
		printf("[Storage] Creando arbol de carpetas 2...\n"); fflush(stdout);
		createTree(a3.c_str(), v10, 3);
	}

	this->field_4 = a2 + "/games" + "/com.mojang" + "/minecraftWorlds";
	this->field_8 = a3 + "/games" + "/com.mojang" + "/minecraftWorlds";
	this->folderName = this->field_8 + "/_LevelCache";

	printf("[Storage] Creando carpeta cache: %s\n", this->folderName.c_str()); fflush(stdout);
	createFolderIfNotExists(this->folderName.c_str());
	printf("[Storage] ExternalFileLevelStorageSource inicializado con exito.\n"); fflush(stdout);
}

void ExternalFileLevelStorageSource::addLevelSummaryIfExists(std::vector<LevelSummary>& a2, const char_t* a3) {
	std::string a1 = this->field_4;
	a1 += '/';
	a1 += a3;
	printf("[Storage] Evaluando mundo en: %s\n", a1.c_str()); fflush(stdout);
	LevelData ld;
	if(ExternalFileLevelStorage::readLevelData(a1, ld)) {
		printf("[Storage] level.dat leido correctamente para: %s\n", a3); fflush(stdout);
		LevelSummary ls;
		ls.field_0 = a3;
		ls.field_10 = ld.getSeed();
		ls.field_4 = ld.levelName;
		ls.field_8 = ld.getLastPlayed();
		ls.field_14 = ld.getSizeOnDisk();
		ls.field_C = ld.getGameType();
		a2.emplace_back(ls);
	} else {
		printf("[Storage] ADVERTENCIA: No se pudo leer level.dat en %s\n", a1.c_str()); fflush(stdout);
	}
}

std::string ExternalFileLevelStorageSource::getFullPath(const std::string& a3) {
	if(LevelStorageSource::TempLevelId == a3) {
		return this->field_8 + '/' + a3;
	} else {
		return this->field_4 + '/' + a3;
	}
}

ExternalFileLevelStorageSource::~ExternalFileLevelStorageSource() {
}

std::string ExternalFileLevelStorageSource::getName() {
	return "External File Level Storage";
}

void ExternalFileLevelStorageSource::getLevelList(std::vector<LevelSummary>& a2) {
	printf("[Storage] getLevelList invocado. Buscando en: %s\n", this->field_4.c_str()); fflush(stdout);
	#ifdef __WIN32__
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile((this->field_4+"\\*.*").c_str(), &data);
	if(hFind == INVALID_HANDLE_VALUE){
		return;
	}
	do{
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			this->addLevelSummaryIfExists(a2, data.cFileName);
		}
	}while(FindNextFile(hFind, &data));
	FindClose(hFind);
	#else
	DIR* v4 = opendir(this->field_4.c_str());
	if(!v4) {
		printf("[Storage] ERROR: opendir fallo al intentar leer la carpeta de mundos.\n"); fflush(stdout);
		return;
	}
	while(1) {
		struct dirent* v6 = readdir(v4);
		if(!v6) {
			break;
		}
		if(v6->d_type == DT_DIR) {
			this->addLevelSummaryIfExists(a2, v6->d_name);
		}
	}
	closedir(v4);
	printf("[Storage] getLevelList finalizado.\n"); fflush(stdout);
	#endif
}

int32_t ExternalFileLevelStorageSource::getDataTagFor(const std::string&) {
	return 0;
}

LevelStorage* ExternalFileLevelStorageSource::selectLevel(const std::string& a2, bool_t a3) {
	printf("[Storage] selectLevel invocado para el nivel: %s\n", a2.c_str()); fflush(stdout);
	return new ExternalFileLevelStorage(a2, this->getFullPath(a2));
}

bool_t ExternalFileLevelStorageSource::isNewLevelIdAcceptable(const std::string&) {
	return 1;
}

void ExternalFileLevelStorageSource::clearAll() {
}

void ExternalFileLevelStorageSource::deleteLevel(const std::string& a2) {
	DeleteDirectory(this->getFullPath(a2), 1);
}

void ExternalFileLevelStorageSource::renameLevel(const std::string& oldName, const std::string& newName) {
	printf("ExternalFileLevelStorageSource::renameLevel - not implemented\n"); fflush(stdout);
}

bool_t ExternalFileLevelStorageSource::isConvertible(const std::string&) {
	return 0;
}

bool_t ExternalFileLevelStorageSource::requiresConversion(const std::string&) {
	return 0;
}

bool_t ExternalFileLevelStorageSource::convertLevel(const std::string&, ProgressListener*) {
	return 0;
}

static std::string _D66784C8(LevelData* a2) { //inlined
	std::stringstream v10;
	v10 << a2->getSeed();
	v10 << '_';
	v10 << a2->getGeneratorVersion();
	v10 << ".cach1";
	return v10.str();
}

void ExternalFileLevelStorageSource::storeToCache(LevelData* a2, const std::string& a3) {
	printf("[Storage] storeToCache invocado...\n"); fflush(stdout);
	std::string v6 = this->folderName + '/' + _D66784C8(a2);
	if(!exists(v6.c_str())) {
		std::string v7 = this->getFullPath(a3);
		std::string v8 = v7 + "/chunks.dat";
		compression::gzip::compress(v8, v6, 1);
	}
}

void ExternalFileLevelStorageSource::loadFromCache(LevelData* a2, const std::string& a3) {
	printf("[Storage] loadFromCache invocado...\n"); fflush(stdout);
	std::string filename = (std::string(this->folderName) + '/') + _D66784C8(a2);
	if(exists(filename.c_str())) {
		std::string v7 = this->getFullPath(a3);
		std::string v8 = v7 + "/chunks.dat";
		if(!compression::gzip::decompress(filename, v8)) {
			remove(filename.c_str());
			remove(v8.c_str());
		}
	}
}

void ExternalFileLevelStorageSource::clearCache() {
}
