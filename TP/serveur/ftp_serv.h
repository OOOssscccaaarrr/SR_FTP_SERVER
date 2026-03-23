#ifndef FTP_SERV_H
#define FTP_SERV_H

#include "../utilitaire/structures.h"

#define NPROC 3


int get(char *nomFichier, int fd_cible);

#endif