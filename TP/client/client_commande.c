#include "../utilitaire/structures.h"

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

int reception_fichier(char nomFichier[MAX_NAME_LEN], reponse_t rep, int clientfd, int fdlog, int paquet_demande, rio_t *rio){
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

    if (compteur_paquet > 0)
        lseek(fd, (off_t)paquet_demande * MAX_PAQ_LEN, SEEK_SET);

    for (; compteur_paquet < nbPaquet; compteur_paquet++){
        
        if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0) {
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
            printf("Attente du paquet numéro : %d / %d\n", paquet_actuelle, rep.nb_paquets);
            if (reception_fichier(nomFichierLocal, rep, clientfd, flog, paquet_actuelle, rio) < rep.nb_paquets){ // Boucle ici pour recevoir les paquets
                printf("ECHEC : Une erreur s'est produite lors de la réception du fichier\n");   
              Close(flog);   
            } else {
                printf("SUCCESS : Fichier %s reçu avec succès\n",nomFichierLocal);
                Close(flog);   
                remove(nomFichierLog);
            }
        } else if (rep.reponse == ENVOIE_FICHIER ||rep.reponse == ENVOIE_TERMINER){
            printf("ECHEC : pas de ACK\n");
            remove(nomFichierLog);
        }
        else {
            gestion_err_serveur(rep.reponse);
            remove(nomFichierLog);
        }
    } else { /* the server has prematurely closed the connection */
        printf("ECHEC : Le serveur a fermé la connexion de manière prématurée\n");
    } 
}


void cmd_ferme(rio_t *rio, request_t req, int clientfd){
    req.type = FERMETURE;
    printf("Déconnexion du serveur ftp...\n");
    Rio_writen(clientfd, &req, sizeof(request_t));

    reponse_t rep;
    if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0 && rep.reponse == ACK){
        printf("Le serveur a confirmé la fermeture\n");
    } else {
        printf("Le serveur n'a pas confirmé la fermeture\n");
    }

}