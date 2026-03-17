/*
 * echoserveri.c - An iterative echo server
 */

#include "ftp_serv.h"

pid_t pid[NPROC];
int pid_pere;

void sigchld_hdlr(int signum){
    waitpid(-1, NULL, WNOHANG);
}

void sigint_hdlr(int signum){
    if (getpid() == pid_pere){
        for (int i = 0; i < NPROC ; i++){
            if (pid[i] != 0){
                kill(pid[i], SIGINT);
                printf("fils tué : %d\n", i);
            }
        }
    }
    exit(0);
}



/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main()
{
    Signal(SIGCHLD, sigchld_hdlr);
    Signal(SIGINT, sigint_hdlr);
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    char buf[MAXBUF];
    pid_pere = getpid();
    for (int i = 0; i < NPROC ; i++){
        pid[i]= 0;
    }

    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT);
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
            Rio_readlineb(&rio, buf, MAXBUF); 
            for (int i = 0; buf[i] != '\0' ;i++){
                if (buf[i] == '\n'){
                    buf[i] = '\0';
                }
            }
            get(buf);
            Close(connfd);
        }
    }
    exit(0);
}

