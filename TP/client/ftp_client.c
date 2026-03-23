/*
 * echoclient.c - An echo client
 */
#include "ftp_client.h"

void gestion_err_serveur( typerep_t reponse){
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

int reception_fichier(char nomFichier[MAX_NAME_LEN], reponse_t rep, int clientfd){
    int fd = Fopen(nomFichier, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0){
        printf("ECHEC : Impossible de créer le fichier %s\n", nomFichier);
        return -1;
    }
    rio_t rio;
    Rio_readinitb(&rio, fd);
    int nbPaquet = rep.nb_paquets;
    int compteur_paquet = 0;
    char buf[MAX_PAQ_LEN];
    for (int compteur_paquet = 0; compteur_paquet < nbPaquet; compteur_paquet++){
        if (Rio_readlineb(clientfd, (char *)&rep, sizeof(reponse_t)) > 0) {
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




int main(int argc, char **argv)
{
    int clientfd;
    char *host, buf[MAXLINE];
    rio_t rio;

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
    printf("client connected to server OS\n"); 
    
    Rio_readinitb(&rio, clientfd);
    reponse_t rep;
    request_t req;
    req.type = GET;
    
    req.num_paquet = 0;
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        for (int i = 0; buf[i] != '\0' ;i++){
                if (buf[i] == '\n'){
                    buf[i] = '\0';
                    break;
                }
            }
        strcpy(req.nomFichier, buf);
        Rio_writen(clientfd, &req, sizeof(request_t));
        if (Rio_readlineb(&rio, (char *)&rep, sizeof(reponse_t)) > 0) {
            if (rep.reponse == ACK){
                printf("SUCCESS : %s\n", req.nomFichier);
                if (reception_fichier(buf, rep, clientfd) < rep.nb_paquets){
                    printf("ECHEC : Une erreur s'est produite lors de la réception du fichier\n");
                } else {
                    printf("SUCCESS : Fichier reçu avec succès\n");
                }
            } else if (rep.reponse == ENVOIE_TERMINER){
                printf("SUCCESS : Envoi du fichier terminé\n");
           
            } else if (rep.reponse == ENVOIE_FICHIER){
                printf("SUCCESS : Envoi du fichier en cours\n");
            }
            else {
                gestion_err_serveur(rep.reponse);
            }
        } else { /* the server has prematurely closed the connection */
            printf("ECHEC : Le serveur a fermé la connexion de manière prématurée\n");
            break;
        }
    }
    Close(clientfd);
    exit(0);
}
