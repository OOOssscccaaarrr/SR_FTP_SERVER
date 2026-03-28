#ifndef FTP_MAITRE_H
#define FTP_MAITRE_H

#include "../utilitaire/structures.h"

#define NB_SLAVE 2


typedef struct {
    char* ip;
    int port;
} serveur_escalve_t;

#endif