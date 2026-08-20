#include <OptionsFile.hpp>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <stdio.h> // Para logs

void OptionsFile::save(const std::vector<std::string>& a2) {
	printf("[OptionsFile] Guardando configuraciones en: %s\n", this->filename.c_str()); fflush(stdout);
	FILE* v3 = fopen(this->filename.c_str(), "w");
	if(v3) {
		for(auto&& s: a2) {
			fprintf(v3, "%s\n", s.c_str());
		}
		fclose(v3);
	} else {
		printf("[ERROR] No se pudo guardar el archivo de opciones.\n"); fflush(stdout);
	}
}
void OptionsFile::setSettingsFolderPath(const std::string& a2) {
	this->filename = a2 + "/options.txt";
}
std::vector<std::string> OptionsFile::getOptionStrings() {
	printf("[OptionsFile] Leyendo configuraciones de: %s\n", this->filename.c_str()); fflush(stdout);
	std::vector<std::string> res;
	FILE* v3 = fopen(this->filename.c_str(), "r");
	if(v3) {
		char s[128];
		while(fgets(s, 128, v3)) {
			if(strlen(s) > 2) {
				std::stringstream v8(s);
				while(!v8.eof()) {
					std::string v7;
					std::getline(v8, v7, ':');

					// Proteccion contra cuelgues si la linea es vacia o tiene espacios extra
					size_t pos = v7.find_last_not_of(" \n\r\t");
					if (pos != std::string::npos) {
						v7.erase(pos + 1);
					} else {
						v7.clear();
					}

					if(!(v7 == "")) {
						res.emplace_back(v7);
					}
				}
			}
		}
		fclose(v3);
	} else {
		printf("[OptionsFile] El archivo de opciones no existe. Se creara mas tarde.\n"); fflush(stdout);
	}

	return res;
}
