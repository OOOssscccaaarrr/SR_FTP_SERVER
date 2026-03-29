#include "ftp_client.h"

/**
 * Lit une ligne depuis stdin et extrait la commande et ses arguments.
 * Commandes supportées : GET <nomFichier>, bye, ls
 * @param buffer       : buffer de sortie pour les arguments de la commande
 * @param taille_buffer : taille du buffer
 * @return 0 (GET), 1 (bye), 3 (ls), -1 (commande invalide ou erreur)
 */
int lecture_ligne(char *buffer, size_t taille_buffer){
    int val_retour = -1;
    char ligne[256];
    if (!fgets(ligne, sizeof(ligne), stdin))
        return -1;
    ligne[strcspn(ligne, "\n")] = '\0';

    char commande[256];
    char nomFichier[256] = {0};

    sscanf(ligne, "%s", commande);  // lit seulement la commande

    char *reste = strchr(ligne, ' ');
    if (reste != NULL) {
        reste++;
        strncpy(nomFichier, reste, sizeof(nomFichier) - 1);
    } else {
        nomFichier[0] = '\0';
    }

    if (strcmp(commande, "bye") == 0)
        val_retour = 1;
    if (strcmp(commande, "ls") == 0)
        val_retour = 3;
    if (strcmp(commande, "GET") == 0)
        val_retour = 0;

    strncpy(buffer, nomFichier, taille_buffer - 1);
    buffer[taille_buffer - 1] = '\0';
    return val_retour;
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
    
    Rio_readinitb(&rio, clientfd);
    
    se = cmd_connexion(&rio, clientfd);
    Close(clientfd);
    if (se.port == 0){
        exit(1);
    }
    else {
        
        clientfd = Open_clientfd(se.ip, se.port);
        Rio_readinitb(&rio, clientfd);
        printf("Connecté au serveur : %s", se.ip);
        printf(":%d\n", se.port);
    }


    while(connexion_ouverte){
        printf("client> ");
        int commande = lecture_ligne(buf, MAX_NAME_LEN);
        strcpy(req.nomFichier, buf);
        switch (commande){
            case 0:
                cmd_get(&rio, req, clientfd);
                break;
            case 1:
                cmd_ferme(&rio, req, clientfd);
                connexion_ouverte = 0;
                break;
            case 3:
                cmd_ls(&rio, req, clientfd);
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
