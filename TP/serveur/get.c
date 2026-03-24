/*
 * echo - read and echo text lines until client closes connection
 */
#include "../utilitaire/structures.h"

int get(char *nomFichier, int fd_cible)
{
    rio_t rio_origine;
    char buf[MAX_PAQ_LEN];
    ssize_t n;
    printf("Demande de lecture de : [%s]\n",nomFichier);
    int fd_origine = open(nomFichier, O_RDONLY, S_IRUSR);
    Rio_readinitb(&rio_origine, fd_origine);

    while ((n = Rio_readlineb(&rio_origine, buf, MAX_PAQ_LEN)) != 0) {
        Rio_writen(fd_cible, buf, n);
        if (n < MAX_PAQ_LEN) {
            Close(fd_origine);
            return 1;
        }
    }

    Close(fd_origine);
    return 0;
}

