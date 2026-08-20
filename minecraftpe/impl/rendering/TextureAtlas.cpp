#include <rendering/TextureAtlas.hpp>
#include <NinecraftApp.hpp>
#include <_AssetFile.hpp>
#include <json/reader.h>
#include <rendering/TextureAtlasTextureItem.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Agregado para memset

TextureAtlas::TextureAtlas(const std::string& s){
	this->path = s;
}

TextureUVCoordinateSet TextureAtlas::_parseJSON(const Json::Value& v){
	TextureUVCoordinateSet t;
	// 1. Inicializar todo en 0 por seguridad
	t.width = 0; t.height = 0;
	t.minX = 0; t.maxX = 0;
	t.minY = 0; t.maxY = 0;

	// 2. Validar que el valor es realmente un array y no nulo
	if (v.isNull() || !v.isArray()) {
		printf("[TextureAtlas] ADVERTENCIA: Nodo UV es nulo o no es un array.\n");
		return t;
	}

	// 3. Validar que tiene los 6 parametros necesarios
	if (v.size() < 6) {
		printf("[TextureAtlas] ADVERTENCIA: Array UV incompleto (tamano: %u).\n", (unsigned int)v.size());
		return t;
	}

	// 4. Bloque seguro por si los tipos de datos en el JSON no son float
	try {
		float v17 = (v[2].asFloat() - v[0].asFloat()) * 0.002f;
		float v18 = (v[3].asFloat() - v[1].asFloat()) * 0.002f;

		t.width = v[4].asFloat();
		t.height = v[5].asFloat();
		t.minX = v[0].asFloat() + v17;
		t.maxX = v[2].asFloat() - v17;
		t.minY = v[1].asFloat() + v18;
		t.maxY = v[3].asFloat() - v18;
	} catch(...) {
		printf("[TextureAtlas] ADVERTENCIA: Excepcion al convertir coordenadas UV a flotante.\n");
	}

	return t;
}

TextureAtlasTextureItem* TextureAtlas::getTextureItem(const std::string& s){
	return &this->field_4[s];
}

void TextureAtlas::load(struct NinecraftApp* mc){
	std::string absolutePath = "/dev_hdd0/game/MCPE00801/USRDIR/assets/" + this->path;
	printf("[TextureAtlas] Intentando abrir: %s\n", absolutePath.c_str());
	fflush(stdout);

	FILE* file = fopen(absolutePath.c_str(), "rb");
	if(!file) {
		printf("[TextureAtlas] ERROR FATAL: fopen fallo. El archivo no existe.\n");
		fflush(stdout);
		return;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if(length <= 0) {
		printf("[TextureAtlas] ERROR FATAL: El archivo esta vacio o es invalido.\n");
		fclose(file);
		fflush(stdout);
		return;
	}

	char* bytes = new char[length + 1];
	size_t readLength = fread(bytes, 1, length, file);
	bytes[readLength] = '\0';
	fclose(file);

	printf("[TextureAtlas] Archivo leido en memoria. Parseando JSON (%ld bytes)...\n", (long)readLength);
	fflush(stdout);

	Json::Reader reader;
	Json::Value root;
	std::string data(bytes, readLength);

	// Try-catch global para que un crasheo del JSON no cierre la PS3 entera
	try {
		if(reader.parse(data, root, false)){
			printf("[TextureAtlas] JSON parseado con exito. Elementos base: %u\n", (unsigned int)root.size());
			fflush(stdout);

			if(!root.isArray()) {
				printf("[TextureAtlas] ERROR: El JSON no es un array valido.\n");
				delete[] bytes;
				return;
			}

			Json::Value::iterator v27 = root.begin();
			Json::Value::iterator v28 = root.end();
			int count = 0;

			while(v27 != v28){
				Json::Value v32(*v27);
				std::vector<TextureUVCoordinateSet> v29;
				std::string v25 = "unknown";

				if (v32.isObject()) {
					// Extraer nombre de forma segura
					if (v32.isMember("name")) {
						v25 = v32["name"].asString();
					}

					// Extraer texturas adicionales de forma segura
					if(v32.isMember("additonal_textures")) {
						Json::Value v6 = v32["additonal_textures"];
						if(!v6.isNull() && v6.isArray()) {
							for(Json::Value::iterator v30 = v6.begin(); v30 != v6.end(); ++v30){
								Json::Value v34(*v30);
								v29.emplace_back(this->_parseJSON(v34));
							}
						}
					}

					// Extraer UV de forma segura (soporta formatos antiguos)
					Json::Value v33_;
					if (v32.isMember("uv")) {
						v33_ = v32["uv"];
					} else if (v32.isMember("uvs")) {
						Json::Value uvs = v32["uvs"];
						if (uvs.isArray() && uvs.size() > 0) {
							v33_ = uvs[0]; // Extraer el primero si es un array 2D
						}
					}

					// Parsear UV y guardar
					if (!v33_.isNull()) {
						TextureUVCoordinateSet v34 = this->_parseJSON(v33_);
						this->field_4[v25] = TextureAtlasTextureItem(v25, v34, v29);
					} else {
						printf("[TextureAtlas] ADVERTENCIA: La textura '%s' no tiene un array UV valido.\n", v25.c_str());
					}
				}

				++v27;
				++count;
			}
			printf("[TextureAtlas] Cargado exitosamente: %s (Total texturas: %u)\n", absolutePath.c_str(), (unsigned int)this->field_4.size());
		} else {
			printf("[TextureAtlas] ERROR SINTACTICO: Fallo al leer el JSON: %s\n", reader.getFormattedErrorMessages().c_str());
		}
	} catch (...) {
		printf("[TextureAtlas] EXCEPCION CRITICA atrapada. Evitando cierre del juego.\n");
	}

	delete[] bytes;
	fflush(stdout);
}
