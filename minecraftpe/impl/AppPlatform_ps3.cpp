#include "AppPlatform_ps3.hpp"
#include <ImageData.hpp>
#include <_AssetFile.hpp>
#include <stb_image.h>
#include <stdio.h>
#include <unistd.h>

// BÚSQUEDA INTELIGENTE DE ARCHIVOS
std::string resolver_ruta_ps3(const std::string& ruta_original) {
	std::string limpia = ruta_original;
	if (limpia.length() > 0 && limpia[0] == '/') {
		limpia = limpia.substr(1);
	}

	// FORZAR RUTAS PARA LOS ARCHIVOS .meta
	// El código original pide "terrain.meta", pero realmente están en "images/terrain.meta"
	if (limpia.length() >= 5 && limpia.substr(limpia.length() - 5) == ".meta") {
		if (limpia.find("images/") != 0) {
			limpia = "images/" + limpia;
		}
	}

	// Intento 1: Estructura original de Android (dentro de assets)
	std::string ruta_assets = "/dev_hdd0/game/MCPE00801/USRDIR/assets/" + limpia;
	if (access(ruta_assets.c_str(), F_OK) != -1) {
		return ruta_assets;
	}

	// Intento 2: Estructura extraída directa (sin assets)
	std::string ruta_directa = "/dev_hdd0/game/MCPE00801/USRDIR/" + limpia;
	return ruta_directa;
}

AppPlatform_ps3::AppPlatform_ps3() : AppPlatform() {
	// [¡VITAL!] Registramos nuestra plataforma en el puntero global del juego
	AppPlatform::_singleton = this;

	printf("[AppPlatform_ps3] Creado puente de plataforma PS3.\n");
	fflush(stdout);
}

AppPlatform_ps3::~AppPlatform_ps3() {}

std::string AppPlatform_ps3::getImagePath(const std::string& path, bool_t a4) {
	std::string fixed_path = path;

	if (fixed_path.find("images/") != 0) {
		fixed_path = "images/" + fixed_path;
	}

	std::string ruta_real = resolver_ruta_ps3(fixed_path);
	return ruta_real;
}

void AppPlatform_ps3::loadPNG(ImageData& data, const std::string& path, bool_t a4) {
	int32_t channels_in_file;
	// Forzar RGBA (4 canales)
	uint8_t* pxls = stbi_load(path.c_str(), &data.width, &data.height, &channels_in_file, 4);

	if (!pxls) {
		printf("[ERROR CRITICO] Fallo al cargar PNG: %s\n", path.c_str());
		fflush(stdout);
	}

	data.field_C = 0;
	data.pixels = pxls;
}

void AppPlatform_ps3::loadTGA(ImageData& data, const std::string& path, bool_t a4) {
	int32_t channels_in_file;
	uint8_t* pxls = stbi_load(path.c_str(), &data.width, &data.height, &channels_in_file, 4);

	if (!pxls) {
		printf("[ERROR CRITICO] Fallo al cargar TGA: %s\n", path.c_str());
		fflush(stdout);
	}

	data.field_C = 0;
	data.pixels = pxls;
}

AssetFile AppPlatform_ps3::readAssetFile(const std::string& path) {
	std::string ruta_real = resolver_ruta_ps3(path);
	return AppPlatform::readAssetFile(ruta_real);
}
