/*
 * echoserveri.c - An iterative echo server
 */

#include "ftp_serv.h"

void sigint_hdlr(int signum){
    exit(0);
}


void reponse_err(int connfd, typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}

void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument){
    if (argument){
        printf("[Fils %d] : %s : %s : %s\n", numero_serveur, client_hostname, message, argument);
        return;
    }
    printf("[Fils %d] : %s : %s\n", numero_serveur, client_hostname, message);
}

char* type_en_char(typereq_t type){
    switch (type){
        case GET:
            return "GET";
        case FERMETURE:
            return "FERMETURE";
        case PUT:
            return "PUT";
        case LS:
            return "LS";
        default:
            return "INCONNU";
    }
}


/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main(int argc, char **argv)
{
    Signal(SIGINT, sigint_hdlr);
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t req;
    reponse_t rep;

     if (argc != 3) {
        fprintf(stderr, "usage: %s <port> <numero_esclave>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    int numero_esclave = atoi(argv[2]);

    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
    while (1) {
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            
        /* determine the name of the client */
        Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        
        /* determine the textual representation of the client's IP address */
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);
        
        afficher_message(numero_esclave, client_hostname, "Connexion du client ", client_ip_string);

        // Initialisation de rio
        rio_t rio;
        Rio_readinitb(&rio, connfd);

        while(1){
            ssize_t n_lu;
            if ( ( n_lu = Rio_readnb(&rio, &req, sizeof(request_t))) != sizeof(request_t)) {
                reponse_err(connfd, ERREUR_REQUETE_INVALIDE);
                afficher_message(numero_esclave, client_hostname, "Erreur lecture requête", NULL);
                break;
            }
            if (n_lu == 0){
                afficher_message(numero_esclave, client_hostname, "Le client a fermé la connexion", NULL);
                break;
            }

            afficher_message(numero_esclave, client_hostname, "requête reçue", type_en_char(req.type));
            afficher_message(numero_esclave, client_hostname, "nom du fichier demandé", req.nomFichier);
            if (req.type == FERMETURE){
                memset(&req, 0, sizeof(request_t));
                // Le client souhaite fermer la connexion
                afficher_message(numero_esclave, client_hostname, "Déconnexion du client", NULL);
                rep.reponse = ACK;
                rep.nb_paquets = 0;
                Rio_writen(connfd, &rep, sizeof(reponse_t));
                break;  
            }
            int fd_origine = open(req.nomFichier, O_RDONLY);
            if (fd_origine < 0) {
                if (errno == ENOENT) {
                    reponse_err(connfd, ERREUR_FICHIER_INEXISTANT);
                    afficher_message(numero_esclave, client_hostname, "Le fichier demandé n'existe pas", NULL);
                    continue;
                } else if (errno == EACCES) {
                    reponse_err(connfd, ERREUR_FICHIER_INACCESSIBLE);
                    afficher_message(numero_esclave, client_hostname, "Le fichier demandé n'est pas accessible", NULL);   
                    continue; 
                } else {
                    reponse_err(connfd, ERREUR_SERVEUR);
                    afficher_message(numero_esclave, client_hostname, "Erreur serveur lors de l'ouverture du fichier", NULL);
                    continue;
                }
                continue;
            }

            struct stat st;
            if (fstat(fd_origine, &st) < 0) {
                perror("fstat");
                reponse_err(connfd, ERREUR_SERVEUR);
                Close(connfd);
                close(fd_origine);
                continue;
            }
            afficher_message(numero_esclave, client_hostname, "Le fichier demandé existe et est accessible", NULL);
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
            afficher_message(numero_esclave, client_hostname, "Envoi des paquets a partir du numéro ",NULL);

            while ((n = Rio_readnb(&rio_origine, buf, MAX_PAQ_LEN)) > 0) {
                //sleep(1); // Simuler un délai de transmission
                if (compteur_paquet >= rep.nb_paquets){ // Erreur a gérer
                    afficher_message(numero_esclave, client_hostname, " [ERREUR] Nombre de paquets dépassé", NULL);
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

            afficher_message(numero_esclave, client_hostname, "Fichier envoyé", NULL);
            Close(fd_origine);
        }
        afficher_message(numero_esclave, client_hostname, "Fermeture de la connexion", NULL);
        Close(connfd);
    }
    exit(0);
}

