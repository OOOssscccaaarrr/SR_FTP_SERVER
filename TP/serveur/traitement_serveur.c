#include "../utilitaire/structures.h"

void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument){
    if (argument){
        printf("[Serveur %d] : %s : %s : %s\n", numero_serveur, client_hostname, message, argument);
        return;
    }
    printf("[Serveur %d] : %s : %s\n", numero_serveur, client_hostname, message);
}

void reponse_err(int connfd, typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}

void traitement_get(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]) {
    reponse_t rep;
    int fd_origine = open(req.nomFichier, O_RDONLY);
    if (fd_origine < 0) {
        if (errno == ENOENT) {
            reponse_err(connfd, ERREUR_FICHIER_INEXISTANT);
            afficher_message(numero_serv, client_hostname, "Le fichier demandé n'existe pas", NULL);
        } else if (errno == EACCES) {
            reponse_err(connfd, ERREUR_FICHIER_INACCESSIBLE);
            afficher_message(numero_serv, client_hostname, "Le fichier demandé n'est pas accessible", NULL);   
        } else {
            reponse_err(connfd, ERREUR_SERVEUR);
            afficher_message(numero_serv, client_hostname, "Erreur serveur lors de l'ouverture du fichier", NULL);
        }
        Close(connfd);
        return;
    }

        struct stat st;
        if (fstat(fd_origine, &st) < 0) {
            perror("fstat");
            reponse_err(connfd, ERREUR_SERVEUR);
            Close(connfd);
            close(fd_origine);
            return;
        }
        afficher_message(numero_serv, client_hostname, "Le fichier demandé existe et est accessible", NULL);
        rep.reponse = ACK;
        
        off_t taille = st.st_size;
        rep.nb_paquets = taille / MAX_PAQ_LEN + (taille % MAX_PAQ_LEN != 0); 
        
        Rio_writen(connfd, &rep, sizeof(reponse_t));

        size_t n;
        char buf[MAX_PAQ_LEN];
        rio_t rio_origine;
        if (req.num_paquet > 0)
            lseek(fd_origine, (off_t)req.num_paquet * MAX_PAQ_LEN, SEEK_SET);
        int compteur_paquet = req.num_paquet;
        Rio_readinitb(&rio_origine, fd_origine);
        afficher_message(numero_serv, client_hostname, "Envoi des paquets a partir du numéro ",NULL);

        while ((n = Rio_readnb(&rio_origine, buf, MAX_PAQ_LEN)) > 0) {
            //sleep(1); // Simuler un délai de transmission
            if (compteur_paquet >= rep.nb_paquets){ // Erreur a gérer
                afficher_message(numero_serv, client_hostname, " [ERREUR] Nombre de paquets dépassé", NULL);
                reponse_err(connfd, ERREUR_SERVEUR);
                Close(connfd)   ;
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

        afficher_message(numero_serv, client_hostname, "Fichier envoyé", NULL);
        Close(fd_origine);
}

void traitement_ls(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]) {
     reponse_t rep;
     rep.reponse = ACK;
     rep.nb_paquets = 0;
     Rio_writen(connfd, &rep, sizeof(reponse_t));
     // TODO : implémenter la commande LS
     afficher_message(numero_serv, client_hostname, "Commande LS reçue, mais pas encore implémentée", NULL);
}