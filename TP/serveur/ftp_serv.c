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


reponse_t reponse_err(int connfd,typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
    Close(connfd);
    return rep;
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

    request_t req;
    reponse_t rep;
    
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
            Rio_readlineb(&rio, &req, MAXBUF);
            if (req.type != GET){
                rep = reponse_err(connfd, ERREUR_REQUETE_INVALIDE);
                printf("Requête de %s invalide\n", client_hostname);
                continue;
            } 
            else {
                int fd_origine = open(req.nomFichier, O_RDONLY, S_IRUSR);
                struct stat st;
                if (fstat(fd_origine, &st) < 0){
                
                }
                if (fd_origine < 0){
                    rep = reponse_err(connfd, ERREUR_FICHIER_INEXISTANT);
                    printf("Le fichier demandé par %s n'existe pas\n", client_hostname);
                    continue;
                }
                if (access(req.nomFichier, R_OK) < 0){
                    rep = reponse_err(connfd, ERREUR_FICHIER_INACCESSIBLE);
                    printf("Le fichier demandé par %s n'est pas accessible\n", client_hostname);
                    continue;
                }
                rep.reponse = ACK;
                
                off_t taille = st.st_size;
                rep.nb_paquets = taille / MAX_PAQ_LEN + (taille % MAX_PAQ_LEN != 0); 
                int compteur_paquet = 0;
                Rio_writen(connfd, &rep, sizeof(reponse_t));


                size_t n;
                char buf[MAX_PAQ_LEN];
                rio_t rio_origine;
                
                Rio_readinitb(&rio_origine, fd_origine);
                while ((n = Rio_readnb(&rio_origine, buf, MAX_PAQ_LEN)) > 0) {
                    if (compteur_paquet >= rep.nb_paquets){ // Erreur a gérer
                        printf("Erreur : nombre de paquets dépassé\n");
                        rep = reponse_err(connfd, ERREUR_SERVEUR);
                        break;
                    }
                    paquet_t paquet;
                    paquet.taille_buffer = n;
                    paquet.numero_paquet = compteur_paquet++;
                    memcpy(paquet.buffer, buf, n);
                    rep.paquet = paquet;
                    rep.reponse = ENVOIE_FICHIER;
                    Rio_writen(connfd, &rep, sizeof(reponse_t));
                }
                Close(fd_origine);
            }
            
            
            Close(connfd);
        }
    }
    exit(0);
}

