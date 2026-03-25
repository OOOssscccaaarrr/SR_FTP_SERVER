/*
 * echoserveri.c - An iterative echo server
 */

#include "ftp_serv.h"

pid_t pid[NPROC];
int pid_pere;

void sigchld_hdlr(int signum){
    waitpid(-1, NULL, WNOHANG);
}

void sigint_hdlr(int signum){
    if (getpid() == pid_pere){
        for (int i = 0; i < NPROC ; i++){
            if (pid[i] == 0){
                kill(pid[i], SIGINT);
                printf("fils tué : %d\n", i);
            }
        }
    }
    exit(0);
}


void reponse_err(int connfd, typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}


char* type_to_string(typereq_t type) {
    switch(type){
        case GET: return "GET";
        case FERMETURE: return "FERMETURE";
        case PUT: return "PUT";
        case LS: return "LS";
        default: return "INCONNU";
    }
}

void afficher_message(int numero_fils, char* client_hostname, char* message, char* argument){
    if (argument){
        printf("[Fils %d] : %s : %s : %s\n", numero_fils, client_hostname, message, argument);
        return;
    }
    printf("[Fils %d] : %s : %s\n", numero_fils, client_hostname, message);
}


/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main()
{
    Signal(SIGCHLD, sigchld_hdlr);
    Signal(SIGINT, sigint_hdlr);
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    request_t req;
    reponse_t rep;
    pid_pere = getpid();

    for (int i = 0; i < NPROC ; i++){
        pid[i]= 0;
    }

    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT);
    for (int i = 0; i < NPROC  && ((pid[i] = Fork()) != 0); i++){
        printf("[PERE] fils crée : %d\n", i);
    }

    int numero_fils = 0;
    if (pid_pere != getpid()){
        while (numero_fils < NPROC && pid[numero_fils] != 0 && pid[numero_fils] != getpid()) {
        numero_fils++;
        }
    }
    while (1) {
        if (pid_pere != getpid()){
            // Fils
           
            

            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                
            /* determine the name of the client */
            Getnameinfo((SA *) &clientaddr, clientlen,
                        client_hostname, MAX_NAME_LEN, 0, 0, 0);
            
            /* determine the textual representation of the client's IP address */
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                    INET_ADDRSTRLEN);
            
            afficher_message(numero_fils, client_hostname, "Connexion du client ", client_ip_string);

            // Initialisation de rio
            rio_t rio;
            Rio_readinitb(&rio, connfd);

            while(1){
                if ( Rio_readnb(&rio, &req, sizeof(request_t)) < sizeof(request_t)) {
                    reponse_err(connfd, ERREUR_REQUETE_INVALIDE);
                    afficher_message(numero_fils, client_hostname, "Erreur lecture requête", NULL);
                    break;
                }

                afficher_message(numero_fils, client_hostname, "requête reçue",type_to_string(req.type));
                if (req.type != FERMETURE) {
                    afficher_message(numero_fils, client_hostname, "nom du fichier demandé", req.nomFichier);
                    }
                else {
                    afficher_message(numero_fils, client_hostname, "Déconnexion du client", NULL);
                    rep.reponse = ACK;
                    rep.nb_paquets = 0;
                    Rio_writen(connfd, &rep, sizeof(reponse_t));
                    break;
                }
                int fd_origine = open(req.nomFichier, O_RDONLY);
                if (fd_origine < 0) {
                    if (errno == ENOENT) {
                        reponse_err(connfd, ERREUR_FICHIER_INEXISTANT);
                        afficher_message(numero_fils, client_hostname, "Le fichier demandé n'existe pas", NULL);
                        continue;
                    } else if (errno == EACCES) {
                        reponse_err(connfd, ERREUR_FICHIER_INACCESSIBLE);
                        afficher_message(numero_fils, client_hostname, "Le fichier demandé n'est pas accessible", NULL);   
                        continue; 
                    } else {
                        reponse_err(connfd, ERREUR_SERVEUR);
                        afficher_message(numero_fils, client_hostname, "Erreur serveur lors de l'ouverture du fichier", NULL);
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
                afficher_message(numero_fils, client_hostname, "Le fichier demandé existe et est accessible", NULL);
                rep.reponse = ACK;
                
                off_t taille = st.st_size;
                rep.nb_paquets = taille / MAX_PAQ_LEN + (taille % MAX_PAQ_LEN != 0); 
                int compteur_paquet = req.num_paquet;
                Rio_writen(connfd, &rep, sizeof(reponse_t));


                size_t n;
                char buf[MAX_PAQ_LEN];
                rio_t rio_origine;
                
                Rio_readinitb(&rio_origine, fd_origine);
                afficher_message(numero_fils, client_hostname, "Envoi des paquets", NULL);
                while ((n = Rio_readnb(&rio_origine, buf, MAX_PAQ_LEN)) > 0) {
                    sleep(1); // aide pour check les crashs
                    if (compteur_paquet >= rep.nb_paquets){ // Erreur a gérer*
                        afficher_message(numero_fils, client_hostname, " [ERREUR] Nombre de paquets dépassé", NULL);
                        reponse_err(connfd, ERREUR_SERVEUR);
                        Close(connfd);
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

                afficher_message(numero_fils, client_hostname, "Fichier envoyé", NULL);
                Close(fd_origine);
            }
            afficher_message(numero_fils, client_hostname, "Fermeture de la connexion", NULL);
            Close(connfd);
        }
        else {
            // père
        }
    }
    exit(0);
}

