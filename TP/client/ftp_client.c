/*
 * echoclient.c - An echo client
 */
#include "ftp_client.h"

void gestion_err_serveur(typerep_t reponse){
    if (reponse == ERREUR_REQUETE_INVALIDE){
        printf("ECHEC : La requête est invalide\n");
    } else if (reponse == ERREUR_FICHIER_INEXISTANT){
        printf("ECHEC : Le fichier demandé n'existe pas\n");
    } else if (reponse == ERREUR_FICHIER_INACCESSIBLE){
        printf("ECHEC : Le fichier n'est pas accessible\n");
    } else if (reponse == ERREUR_REQUETE_INVALIDE){
        printf("ECHEC : La requête est invalide\n");
    } else if (reponse == ERREUR_SERVEUR){
        printf("ECHEC : Une erreur s'est produite sur le serveur\n");
    }
}

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

int reception_fichier(char nomFichier[MAX_NAME_LEN], reponse_t rep, int clientfd){
    int fd = Open(nomFichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0){
        printf("ECHEC : Impossible de créer le fichier %s\n", nomFichier);
        return -1;
    }
    rio_t rio;
    Rio_readinitb(&rio, clientfd);
    int nbPaquet = rep.nb_paquets;
    int compteur_paquet = 0;
    for (; compteur_paquet < nbPaquet; compteur_paquet++){
        
        if (Rio_readnb(&rio, &rep, sizeof(reponse_t)) > 0) {
            if (rep.reponse == ENVOIE_FICHIER){
                Rio_writen(fd, rep.paquet.buffer, rep.paquet.taille_buffer);
            }
            else if (rep.reponse == ENVOIE_TERMINER){
                printf("SUCCESS : Envoi du fichier terminé\n");
                break;
            }
            else {
                gestion_err_serveur(rep.reponse);
                break;
            }
        } else {
            printf("ECHEC : Le serveur a fermé la connexion de manière prématurée\n");
            break;
        }
    }
    Close(fd);
    return compteur_paquet;
}



void cmd_get(rio_t rio, request_t req, int clientfd){
    
    reponse_t rep;
    char *nomFichierLocal = strrchr(req.nomFichier, '/');
    if (nomFichierLocal != NULL)
        nomFichierLocal++;
    else
    nomFichierLocal = req.nomFichier;
    if (Rio_readnb(&rio, &rep, sizeof(reponse_t)) > 0) {
        if (rep.reponse == ACK){
            if (reception_fichier(nomFichierLocal, rep, clientfd) < rep.nb_paquets){ // Boucle ici pour recevoir les paquets
                printf("ECHEC : Une erreur s'est produite lors de la réception du fichier\n");
            } else {
                printf("SUCCESS : Fichier %s reçu avec succès\n",nomFichierLocal);
            }
        } else if (rep.reponse == ENVOIE_FICHIER ||rep.reponse == ENVOIE_TERMINER){
            printf("ECHEC : pas de ACK\n");
        }
        else {
            gestion_err_serveur(rep.reponse);
        }
    } else { /* the server has prematurely closed the connection */
        printf("ECHEC : Le serveur a fermé la connexion de manière prématurée\n");
    }
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
    clientfd = Open_clientfd(host, PORT);
    
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
                req.type = GET;
                strcpy(req.nomFichier, buf);
                Rio_writen(clientfd, &req, sizeof(request_t)); // Envoie de la première requête au serveur
                cmd_get(rio, req, clientfd);
                break;
            case 1:
                req.type = FERMETURE;
                printf("Déconnexion du serveur ftp\n");
                connexion_ouverte = 0;
                Rio_writen(clientfd, &req, sizeof(request_t));
                break;
            default:
                printf("ECHEC : Commande invalide. Entrez une commande au format 'GET <nomFichier>' ou 'bye'\n");
                continue;
        }
    }      
    Close(clientfd);
    exit(0);
}
