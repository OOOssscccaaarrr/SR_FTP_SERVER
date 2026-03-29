#include <ftp_maitre.h>

/**
 * Envoie une réponse d'erreur ERREUR_REQUETE_INVALIDE au client.
 * @param rio    : buffer de lecture rio (non utilisé ici)
 * @param connfd : file descriptor de la connexion client
 */
void requete_invalide(rio_t *rio, int connfd){
    reponse_t rep;
    rep.reponse = ERREUR_REQUETE_INVALIDE;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    rep.serveur_esclave = (serveur_esclave_t) {0};
    printf("[MAITRE] type de requete invalide\n");
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}

int main(){
    int listenfd, connfd, round_robin = 0;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];

    serveur_esclave_t tbl_esclave[NB_SLAVE];
    request_t req;
    reponse_t rep;

    clientlen = (socklen_t)sizeof(clientaddr);


    for (int i = 0 ; i < NB_SLAVE; i++){
        strncpy(tbl_esclave[i].ip, "127.0.0.1", INET_ADDRSTRLEN);
        tbl_esclave[i].port = PORT_DEFAUT + i + 1;
        char cmd_serv_esclave[256];
        snprintf(cmd_serv_esclave, sizeof(cmd_serv_esclave), "./ftp_serv %d %d &",
            tbl_esclave[i].port, i + 1);
        system(cmd_serv_esclave); //TODO ARGUEMNT
        printf("[MAITRE] Démarrage de l'esclave n°%d  %s:%d\n", i + 1,tbl_esclave[i].ip, tbl_esclave[i].port);
    }

    sleep(1);  // laisser le temps aux esclaves de démarrer
    listenfd = Open_listenfd(PORT_DEFAUT);

    while(1){
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Getnameinfo((SA *) &clientaddr, clientlen,
                        client_hostname, MAX_NAME_LEN, 0, 0, 0);
            
        /* determine the textual representation of the client's IP address */
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);

        printf("[MAITRE] Connection du client : %s, %s\n", client_hostname, client_ip_string);

        rio_t rio;
        Rio_readinitb(&rio, connfd);
        if ( Rio_readnb(&rio, &req, sizeof(request_t)) != sizeof(request_t)) {
            requete_invalide(&rio, connfd); 
        }
        else if (req.type == REQUETE_REDIRECTION){
            int numero_esclave = round_robin % NB_SLAVE;
            round_robin++;
            rep.reponse = REDIRECTION;
            rep.nb_paquets = 0;
            rep.paquet = (paquet_t) {0};
            rep.serveur_esclave = tbl_esclave[numero_esclave];
            printf("[MAITRE] Redirection vers l'esclave n°%d\n",numero_esclave +1);
            Rio_writen(connfd, &rep, sizeof(reponse_t));
        } else {
           requete_invalide(&rio, connfd);
        }

        Close(connfd);


    }


}