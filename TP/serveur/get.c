/*
 * echo - read and echo text lines until client closes connection
 */
#include "../utilitaire/csapp.h"

void get(char *nomFichier)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    printf("Demande de lecture de : [%s]\n",nomFichier);
    int fd = Open(nomFichier, O_RDONLY, S_IRUSR);
    Rio_readinitb(&rio, fd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("%s", buf);
    }
}

