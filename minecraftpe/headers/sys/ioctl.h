#pragma once

// Parche para RakNet en PS3:
// La PS3 no tiene <sys/ioctl.h> nativo de Linux.
// Engañamos a RakNet con un ioctl falso que devuelve error/vacío.

#define FIONREAD 0x541B
#define SIOCGIFHWADDR 0x8927
#define SIOCGIFBRDADDR 0x8919

#ifdef __cplusplus
extern "C" {
    #endif

    // Funcion falsa de control de I/O
    static inline int ioctl(int fd, unsigned long request, ...) {
        return -1; // Siempre devuelve -1 (operacion no soportada)
    }

    #ifdef __cplusplus
}
#endif
