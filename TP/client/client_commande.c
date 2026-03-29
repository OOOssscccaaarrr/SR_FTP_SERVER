#include "../utilitaire/structures.h"

/**
 * Affiche un message d'erreur correspondant au code d'erreur reçu du serveur.
 * @param reponse : le code d'erreur reçu du serveur
 */
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

/**
 * Vérifie l'existence d'un fichier log pour une reprise de transfert.
 * Si le log existe : ouvre en lecture/écriture et retourne le numéro du prochain paquet attendu.
 * Si le log n'existe pas : crée le fichier log et retourne 0.
 * @param flog : pointeur vers le file descriptor du log
 * @param nomFichierLog : nom du fichier log
 * @return le numéro du prochain paquet attendu, -1 en cas d'erreur
 */
int checklog(int *flog, char nomFichierLog[MAX_NAME_LEN + 5]){
    if (access(nomFichierLog, F_OK) == 0) {
        *flog = Open(nomFichierLog, O_RDWR, S_IRUSR | S_IWUSR);
        if (*flog < 0){
            printf("ECHEC : Impossible d'ouvrir le fichier log %s\n", nomFichierLog);
            remove(nomFichierLog);
            return -1;
        }
        lseek(*flog, 0, SEEK_SET);
        char contenu_log[256];
        if (read(*flog, contenu_log, sizeof(contenu_log)) > 0){
            int dernier_paquet_recu = atoi(contenu_log);
            return dernier_paquet_recu + 1;
        } else {        
            printf("ECHEC : Impossible de lire le fichier log pour la reprise de paquets\n");
            remove(nomFichierLog);
            return 0; // vide
        }
    }
    else {
        *flog = Open(nomFichierLog, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (*flog < 0){
            printf("ECHEC : Impossible d'ouvrir le fichier log %s\n", nomFichierLog);
            remove(nomFichierLog);
            return -1;
        }
        return 0;
    }
    return 0;
}

/**
 * Reçoit les paquets du serveur et les écrit dans un fichier local.
 * Ouvre le fichier en écriture (sans troncature si reprise).
 * Se positionne au bon offset avec lseek si reprise.
 * Met à jour le log après chaque paquet reçu.
 * @param nomFichier : nom du fichier local de destination
 * @param rep : dernière réponse reçue du serveur (contient nb_paquets)
 * @param clientfd : file descriptor de la connexion
 * @param fdlog : file descriptor du fichier log
 * @param paquet_demande : numéro du premier paquet attendu
 * @param rio : buffer de lecture rio
 * @return le nombre de paquets reçus, -1 en cas d'erreur
 */
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


/**
 * Gère la commande GET complète côté client.
 * Vérifie le log pour une éventuelle reprise.
 * Envoie la requête GET avec le numéro du premier paquet attendu.
 * Reçoit l'ACK puis appelle reception_fichier.
 * Supprime le log en cas de succès et affiche les statistiques (taille, durée, débit).
 * @param rio : buffer de lecture rio
 * @param req : requête à envoyer (contient le nom du fichier)
 * @param clientfd : file descriptor de la connexion
 */
void cmd_get(rio_t *rio, request_t req, int clientfd){

    time_t deb_time;
    time(&deb_time);
    
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

    mkdir("receipt", 0755);
    char chemin_receipt[MAX_PATH_LEN];
    snprintf(chemin_receipt, sizeof(chemin_receipt), "receipt/%s", nomFichierLocal);
    char *nomFichierFinal = chemin_receipt;

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
            if (reception_fichier(nomFichierFinal, rep, clientfd, flog, paquet_actuelle, rio) < rep.nb_paquets){ // Boucle ici pour recevoir les paquets
                printf("ECHEC : Une erreur s'est produite lors de la réception du fichier\n");   
              Close(flog);   
            } else {
                printf("Fichier %s reçu avec succès\n",nomFichierFinal);
                Close(flog);   
                remove(nomFichierLog);
                time_t fin_time;
                time(&fin_time);
                double y_t = difftime(fin_time, deb_time);
                int x_b = rep.nb_paquets * MAX_PAQ_LEN;
                double z_b = (x_b / 1024.0) / (y_t > 0 ? y_t :1);
                printf("Reçu %d bytes en %f secondes (%f Kbytes/s). \n", x_b, y_t, z_b);
            }
        } else if (rep.reponse == ENVOIE_FICHIER){
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

/**
 * Envoie une requête de fermeture au serveur et attend sa confirmation (ACK).
 * @param rio : buffer de lecture rio
 * @param req : requête à envoyer
 * @param clientfd : file descriptor de la connexion
 */
void cmd_ferme(rio_t *rio, request_t req, int clientfd){
    memset(&req, 0, sizeof(request_t));
    req.type = FERMETURE;
    printf("Déconnexion du serveur ftp\n");
    Rio_writen(clientfd, &req, sizeof(request_t));

    reponse_t rep;
    if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0 && rep.reponse == ACK){
        printf("Le serveur a confirmé la deconnexion\n");
    } else {
        printf("ECHEC : Le serveur n'a pas confirmé la deconnexion\n");
    }

}

/**
 * Envoie une requête de redirection au serveur maître.
 * Retourne les informations de connexion de l'esclave assigné.
 * @param rio : buffer de lecture rio
 * @param clientfd : file descriptor de la connexion au maître
 * @return serveur_esclave_t contenant l'ip et le port de l'esclave, port=0 en cas d'échec
 */
serveur_esclave_t cmd_connexion(rio_t *rio, int clientfd){
    request_t req;
    memset(&req, 0, sizeof(request_t));
    req.type = REQUETE_REDIRECTION;
    Rio_writen(clientfd, &req, sizeof(request_t));

    reponse_t rep;
    if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0 && rep.reponse == REDIRECTION){
        printf("Redirection reçu\n");
        return rep.serveur_esclave;
    } else {
        printf("ECHEC : Redirection non reçu\n");
        return (serveur_esclave_t){.ip = "", .port = 0};
    }
}

/**
 * Envoie une requête LS au serveur avec les arguments dans nomFichier.
 * Reçoit le résultat paquet par paquet et l'affiche directement sur stdout.
 * @param rio : buffer de lecture rio
 * @param req : requête à envoyer (contient les arguments ls dans nomFichier)
 * @param clientfd : file descriptor de la connexion
 */
void cmd_ls(rio_t *rio, request_t req, int clientfd){
    req.type = LS;
    // nomFichier peut contenir les arguments ex: "-la"
    Rio_writen(clientfd, &req, sizeof(request_t));

    reponse_t rep;
    if (Rio_readnb(rio, &rep, sizeof(reponse_t)) <= 0 || rep.reponse != ACK){
        printf("ECHEC : pas de ACK\n");
        return;
    }

    int nbPaquet = rep.nb_paquets;
    for (int i = 0; i < nbPaquet; i++){
        if (Rio_readnb(rio, &rep, sizeof(reponse_t)) > 0 && rep.reponse == ENVOIE_FICHIER){
            // Afficher directement sur stdout au lieu d'écrire dans un fichier
            write(STDOUT_FILENO, rep.paquet.buffer, rep.paquet.taille_buffer);
        } else {
            printf("ECHEC : erreur réception paquet ls\n");
            break;
        }
    }

}