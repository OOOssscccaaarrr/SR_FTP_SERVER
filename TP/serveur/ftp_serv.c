/*
 * echoserveri.c - An iterative echo server
 */

#include "ftp_serv.h"

void sigint_hdlr(int signum){
    exit(0);
}


void reponse_err(int connfd, typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}

void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument){
    if (argument){
        printf("[Serveur %d] : %s : %s : %s\n", numero_serveur, client_hostname, message, argument);
        return;
    }
    printf("[Serveur %d] : %s : %s\n", numero_serveur, client_hostname, message);
}

char* type_en_char(typereq_t type){
    switch (type){
        case GET:
            return "GET";
        case FERMETURE:
            return "FERMETURE";
        case LS:
            return "LS";
        default:
            return "INCONNU";
    }
}


/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main(int argc, char **argv)
{
    Signal(SIGINT, sigint_hdlr);
    int listenfd, connfd, port, connecte = 1;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t req;
    reponse_t rep;

     if (argc != 3) {
        fprintf(stderr, "usage: %s <port> <numero_esclave>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    int numero_esclave = atoi(argv[2]);

    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
    while (1) {
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            
        /* determine the name of the client */
        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        
        /* determine the textual representation of the client's IP address */
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);
        
        afficher_message(numero_esclave, client_hostname, "Connexion du client ", client_ip_string);

        // Initialisation de rio
        rio_t rio;
        Rio_readinitb(&rio, connfd);

        while(connecte){
            ssize_t n_lu = Rio_readnb(&rio, &req, sizeof(request_t));
            if (n_lu == 0) {
                afficher_message(numero_esclave, client_hostname, "Client déconnecté brutalement", NULL);
                break;
            } else if (n_lu != sizeof(request_t)) {
                reponse_err(connfd, ERREUR_REQUETE_INVALIDE);
                break;
            }

            afficher_message(numero_esclave, client_hostname, "requête reçue", type_en_char(req.type));
            afficher_message(numero_esclave, client_hostname, "nom du fichier demandé", req.nomFichier);
            switch (req.type)
            {
            case GET:
                traitement_get(&rio, connfd, numero_esclave, client_hostname, req.nomFichier);
                break;
            case FERMETURE:
                memset(&req, 0, sizeof(request_t));
                // Le client souhaite fermer la connexion
                afficher_message(numero_esclave, client_hostname, "Déconnexion du client", NULL);
                rep.reponse = ACK;
                rep.nb_paquets = 0;
                Rio_writen(connfd, &rep, sizeof(reponse_t));
                connecte = 0;
                break;
            case LS:
                /* code */
                break;
            default:
                reponse_err(connfd, ERREUR_REQUETE_INVALIDE);
                break;
            }
           
        }
        afficher_message(numero_esclave, client_hostname, "Fermeture de la connexion", NULL);
        Close(connfd);
    }
    exit(0);
}

