#ifndef FTP_SERV_H
#define FTP_SERV_H

#include "../utilitaire/structures.h"

#define NPROC 3

void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument);
void reponse_err(int connfd, typerep_t type);
void traitement_get(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]);
void traitement_ls(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]);

#endif