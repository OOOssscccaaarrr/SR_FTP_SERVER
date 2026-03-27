/*
 * echoclient.c - An echo client
 */
#include "ftp_client.h"

int clientfd_global;


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


int checklog(int *flog, char nomFichierLog[MAX_NAME_LEN + 5]){
    if (access(nomFichierLog, F_OK) == 0) {
        *flog = Open(nomFichierLog, O_RDWR, S_IRUSR | S_IWUSR);
        if (*flog < 0){
            printf("ECHEC : Impossible d'ouvrir le fichier log %s\n", nomFichierLog);
            return -1;
        }
        lseek(*flog, 0, SEEK_SET);
        char contenu_log[256];
        if (read(*flog, contenu_log, sizeof(contenu_log)) > 0){
            int dernier_paquet_recu = atoi(contenu_log);
            return dernier_paquet_recu + 1;
        } else {        
            printf("ECHEC : Impossible de lire le fichier log pour la reprise de paquets\n");
            return 0; // vide
        }
    }
    else {
        *flog = Open(nomFichierLog, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (*flog < 0){
            printf("ECHEC : Impossible d'ouvrir le fichier log %s\n", nomFichierLog);
            return -1;
        }
        return 0;
    }
    return 0;
}

int reception_fichier(char nomFichier[MAX_NAME_LEN], reponse_t rep, int clientfd, int fdlog, int paquet_demande){
    rio_t rio_file;
    
    int nbPaquet = rep.nb_paquets;
    int compteur_paquet = paquet_demande;
    int flag, fd;

    if (compteur_paquet >= nbPaquet){
        printf("Le fichier est déjà complètement reçu\n");
        return nbPaquet;
    }

    if (compteur_paquet > 0){
        flag = O_WRONLY | O_CREAT;
    }
    else {
        flag = O_WRONLY | O_CREAT | O_TRUNC;
    }


    fd = Open(nomFichier, flag, S_IRUSR | S_IWUSR);
    if (fd < 0){
        printf("ECHEC : Impossible de créer le fichier %s\n", nomFichier);
        return -1;
    }

    Rio_readinitb(&rio_file, clientfd);
    if (compteur_paquet > 0)
        lseek(fd, (off_t)paquet_demande * MAX_PAQ_LEN, SEEK_SET);


    for (; compteur_paquet < nbPaquet; compteur_paquet++){
        
        if (Rio_readnb(&rio_file, &rep, sizeof(reponse_t)) > 0) {
            if (rep.reponse == ENVOIE_FICHIER){
                Rio_writen(fd, rep.paquet.buffer, rep.paquet.taille_buffer);
                
                char contenu_log[256];
                sprintf(contenu_log, "%d", rep.paquet.numero_paquet);
                lseek(fdlog, 0 , SEEK_SET);
                write(fdlog, contenu_log, strlen(contenu_log));   
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



void cmd_get(rio_t *rio, request_t req, int clientfd){
    
    reponse_t rep;
    int flog, paquet_actuelle = 0;
    req.type = GET;
    char *nomFichierLocal = strrchr(req.nomFichier, '/');
    if (nomFichierLocal != NULL){ 
        nomFichierLocal++;
    }
    else {
    nomFichierLocal = req.nomFichier;
    }


    // LOG
    char dot_log[] = ".log";
    char nomFichierLog[MAX_NAME_LEN + 5];
    strcpy(nomFichierLog, nomFichierLocal);
    strcat(nomFichierLog, dot_log);

    paquet_actuelle = checklog(&flog, nomFichierLog);
    if (paquet_actuelle < 0){
        return;
    }
    req.num_paquet = paquet_actuelle;
    
    // REQUETE
    Rio_writen(clientfd, &req, sizeof(request_t));
    if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0) {
        if (rep.reponse == ACK){

             printf("Attente du premier paquet : %d / %d\n", paquet_actuelle, rep.nb_paquets);
            if (reception_fichier(nomFichierLocal, rep, clientfd, flog, paquet_actuelle) < rep.nb_paquets){ // Boucle ici pour recevoir les paquets
                printf("ECHEC : Une erreur s'est produite lors de la réception du fichier\n");   
              Close(flog);   
            } else {
                printf("SUCCESS : Fichier %s reçu avec succès\n",nomFichierLocal);
                Close(flog);   
                remove(nomFichierLog);
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
                Rio_writen(clientfd, &req, sizeof(request_t));

                reponse_t rep;
                if (Rio_readnb(&rio, &rep, sizeof(reponse_t)) > 0 && rep.reponse == ACK){
                    printf("Le serveur a confirmé la fermeture\n");
                } else {
                    printf("Le serveur n'a pas confirmé la fermeture\n");
                }

                break;
            default:
                printf("ECHEC : Commande invalide. Entrez une commande au format 'GET <nomFichier>' ou 'bye'\n");
                continue;
        }
    }      
    Close(clientfd);
    exit(0);
}
