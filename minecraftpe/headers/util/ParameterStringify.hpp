#pragma once
#include <_types.h>
#include <vector>
#include <string>
#include <stdio.h> // Anadido para el printf

struct ParameterStringify
{
	// ARREGLADO: Quitamos la pereza del descompilador y dejamos el cuerpo de la funcion
	template<typename... _args>
	static void stringifyNext(std::vector<std::string>&, _args... args) {
		printf("ParameterStringify::stringifyNext - not implemented\n");
	}
};
