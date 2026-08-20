#pragma once
#include <netinet/in.h>

// Parche para RakNet en PS3:
// Falsificamos las banderas y estructuras de red de Linux.
#define IFF_UP 0x1
#define IFF_BROADCAST 0x2
#define IFF_LOOPBACK 0x8
#define IFF_POINTOPOINT 0x10
#define IFF_MULTICAST 0x1000

#define IFNAMSIZ 16
#define SIOCGIFCONF 0x8912
#define SIOCGIFNETMASK 0x891b

// El sockaddr de PS3 usa un "flexible array" que rompe el compilador si se anida.
// Creamos nuestra propia version estatica de 16 bytes (el tamano real) para RakNet.
struct ps3_sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

// Estructuras de red falsificadas y seguras
struct ifreq {
    char ifr_name[IFNAMSIZ];
    struct ps3_sockaddr ifr_addr;
    struct ps3_sockaddr ifr_netmask;
};

struct ifconf {
    int ifc_len;
    // Union permite que ifc_buf e ifc_req compartan el mismo espacio
    union {
        char *ifc_buf;
        struct ifreq *ifc_req;
    };
};
