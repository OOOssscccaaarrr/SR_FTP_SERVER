/*
 * echoserveri.c - An iterative echo server
 */

#include "csapp.h"

#define MAX_NAME_LEN 256
#define NPROC 3

void ftp_echo(char *nomFichier);

void handler(int signum){
    waitpid(-1, NULL, WNOHANG);
}


/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main(int argc, char **argv)
{
    Signal(SIGCHLD, handler);
    pid_t pid[NPROC];
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    char buf[1024];
    int pid_pere = getpid();
    for (int i = 0; i < NPROC ; i++){
        pid[i]= 0;
    }
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
    for (int i = 0; i < NPROC  && ((pid[i] = Fork()) != 0); i++){
        printf("fils crée : %d\n", i);
    }
    
    while (1) {
        if (pid_pere != getpid()){

        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            
        /* determine the name of the client */
        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        
        /* determine the textual representation of the client's IP address */
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);
        
        printf("server connected to %s (%s)\n", client_hostname,
            client_ip_string);
        rio_t rio;
        Rio_readinitb(&rio, connfd);
        Rio_readlineb(&rio, buf, 1024); 
        for (int i = 0; buf[i] != '\0' ;i++){
            if (buf[i] == '\n'){
                buf[i] = '\0';
            }
        }
        ftp_echo(buf);
        Close(connfd);
        }
    }
    exit(0);
}

