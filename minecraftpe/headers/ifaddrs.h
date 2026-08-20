#pragma once

// Parche para RakNet en PS3:
// La PS3 no tiene <ifaddrs.h> nativo para listar interfaces de red.
// Creamos una estructura falsa para que compile sin problemas.

struct sockaddr;

struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    unsigned int     ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_dstaddr;
    void            *ifa_data;
};

// Funciones falsas que devuelven error instantaneo para que no intente buscar IPs
static inline int getifaddrs(struct ifaddrs **ifap) { *ifap = 0; return -1; }
static inline void freeifaddrs(struct ifaddrs *ifa) { }
