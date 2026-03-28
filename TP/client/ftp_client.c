#include "ftp_client.h"

int lecture_ligne(char *buffer, size_t taille_buffer){
    char ligne[256];
    if (!fgets(ligne, sizeof(ligne), stdin))
        return -1;
    ligne[strcspn(ligne, "\n")] = '\0';

    char commande[256];
    char nomFichier[256] = {0};

    sscanf(ligne, "%s %s", commande, nomFichier);  // lit les deux mots d'un coup

    if (strcmp(commande, "bye") == 0 || strcmp(commande, "q") == 0)
        return 1;
    if (strcmp(commande, "GET") != 0)
        return -1;

    strncpy(buffer, nomFichier, taille_buffer - 1);
    buffer[taille_buffer - 1] = '\0';
    return 0;
}





int main(int argc, char **argv)
{
    int clientfd, connexion_ouverte = 1;
    
    char *host, buf[MAXLINE];
    rio_t rio;
    request_t req;
    serveur_esclave_t se;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host> \n", argv[0]);
        exit(0);
    }

    host = argv[1];
    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, PORT_DEFAUT);

    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("Connecté au serveur ftp\n"); 
    
    Rio_readinitb(&rio, clientfd);
    
    se = cmd_connexion(&rio, clientfd);
    Close(clientfd);
    if (se.port == 0){
        exit(1);
    }
    else {
        printf("Redirection vers le serveur esclave : %s", se.ip);
        printf(":%d\n", se.port);
        clientfd = Open_clientfd(se.ip, se.port);
        printf("Connecté au serveur esclave ftp\n");
        Rio_readinitb(&rio, clientfd);
    }


    while(connexion_ouverte){
        printf("client> ");
        int commande = lecture_ligne(buf, MAX_NAME_LEN);

        switch (commande){
            case 0:
                strcpy(req.nomFichier, buf);
                cmd_get(&rio, req, clientfd);
                break;
            case 1:
                cmd_ferme(&rio, req, clientfd);
                connexion_ouverte = 0;

                break;
            default:
                printf("ECHEC : Commande invalide. Entrez une commande au format 'GET <nomFichier>' ou 'bye'\n");
                continue;
        }
        // Réinitialisation de req et rep pour la prochaine requete
        memset(&req, 0, sizeof req);
    }      
    Close(clientfd);
    exit(0);
}
