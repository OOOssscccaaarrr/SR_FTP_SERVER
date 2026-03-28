/*
 * echoclient.c - An echo client
 */
#include "ftp_client.h"

int clientfd_global;




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

    clientfd_global = clientfd;
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("Connecté au serveur ftp\n"); 
    
    Rio_readinitb(&rio, clientfd);
    // Première requête
    req.num_paquet = 0;


    while(connexion_ouverte){
        printf("client> ");
        int commande = lecture_ligne(buf, MAX_NAME_LEN);

        switch (commande){
            case 0:
                strcpy(req.nomFichier, buf);
                cmd_get(&rio, req, clientfd);
                break;
            case 1:
                req.type = FERMETURE;
                printf("Déconnexion du serveur ftp...\n");
                connexion_ouverte = 0;
                cmd_ferme(&rio, req, clientfd);

                break;
            default:
                printf("ECHEC : Commande invalide. Entrez une commande au format 'GET <nomFichier>' ou 'bye'\n");
                continue;
        }
    }      
    Close(clientfd);
    exit(0);
}
