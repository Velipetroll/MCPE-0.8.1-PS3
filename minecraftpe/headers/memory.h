#pragma once

// Parche para RakNet en PS3:
// RakNet busca <memory.h> (comun en Windows antiguos),
// pero en PS3 (y C/C++ modernos) las funciones de memoria estan en <string.h>.
#include <string.h>
