#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include "../utilitaire/structures.h"

void cmd_get(rio_t *rio, request_t req, int clientfd);
void cmd_ferme(rio_t *rio, request_t req, int clientfd);


#endif