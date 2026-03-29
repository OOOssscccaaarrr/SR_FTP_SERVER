#include "../utilitaire/structures.h"

/**
 * Affiche un message formaté avec le numéro du serveur et le nom du client.
 * Format : "[Serveur <numero>] : <client> : <message> : <argument>" si argument non NULL
 *          "[Serveur <numero>] : <client> : <message>" sinon
 * @param numero_serveur  : numéro d'identification du serveur esclave
 * @param client_hostname : nom d'hôte du client
 * @param message         : message à afficher
 * @param argument        : argument optionnel (NULL si absent)
 */
void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument){
    if (argument){
        printf("[Serveur %d] : %s : %s : %s\n", numero_serveur, client_hostname, message, argument);
        return;
    }
    printf("[Serveur %d] : %s : %s\n", numero_serveur, client_hostname, message);
}

/**
 * Envoie une réponse d'erreur au client.
 * @param connfd : file descriptor de la connexion client
 * @param type   : type d'erreur à envoyer
 */
void reponse_err(int connfd, typerep_t type){
    reponse_t rep;
    rep.reponse = type;
    rep.nb_paquets = 0;
    rep.paquet = (paquet_t) {0};
    Rio_writen(connfd, &rep, sizeof(reponse_t));
}

/**
 * Traite une requête GET : ouvre le fichier demandé, envoie un ACK avec le nombre
 * de paquets, puis transmet le fichier paquet par paquet via ENVOIE_FICHIER.
 * Supporte la reprise à partir d'un numéro de paquet donné via req.num_paquet
 * en se positionnant au bon offset avec lseek.
 * @param rio             : buffer de lecture rio (non utilisé ici)
 * @param connfd          : file descriptor de la connexion client
 * @param req             : requête reçue (contient nomFichier et num_paquet)
 * @param numero_serv     : numéro d'identification du serveur esclave
 * @param client_hostname : nom d'hôte du client
 */
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
        return;
    }

        struct stat st;
        if (fstat(fd_origine, &st) < 0) {
            perror("fstat");
            reponse_err(connfd, ERREUR_SERVEUR);
            close(fd_origine);
            return;
        }
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
            if (compteur_paquet >= rep.nb_paquets){ 
                afficher_message(numero_serv, client_hostname, " [ERREUR] Nombre de paquets dépassé", NULL);
                reponse_err(connfd, ERREUR_SERVEUR);
                Close(connfd)  ;
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


/**
 * Traite une requête LS : exécute la commande ls avec les arguments fournis
 * dans req.nomFichier via popen, lit la sortie et la transmet au client
 * paquet par paquet via le même protocole que GET.
 * @param rio             : buffer de lecture rio (non utilisé ici)
 * @param connfd          : file descriptor de la connexion client
 * @param req             : requête reçue (contient les arguments ls dans nomFichier)
 * @param numero_serv     : numéro d'identification du serveur esclave
 * @param client_hostname : nom d'hôte du client
 */
void traitement_ls(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]) {
    char cmd[MAX_NAME_LEN + 4];
    if (strlen(req.nomFichier) > 0)
        snprintf(cmd, sizeof(cmd), "ls %s", req.nomFichier);
    else
        snprintf(cmd, sizeof(cmd), "ls");

    char resultat[65536] = {0};
    int total = 0, n;

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        reponse_err(connfd, ERREUR_SERVEUR);
        return;
    }
    while ((n = fread(resultat + total, 1, sizeof(resultat) - total, fp)) > 0)
        total += n;
    pclose(fp);

    reponse_t rep;
    rep.reponse = ACK;
    rep.nb_paquets = total / MAX_PAQ_LEN + (total % MAX_PAQ_LEN != 0);
    Rio_writen(connfd, &rep, sizeof(reponse_t));

    int offset = 0, compteur = 0;
    while (offset < total) {
        int taille = (total - offset < MAX_PAQ_LEN) ? total - offset : MAX_PAQ_LEN;
        paquet_t paquet;
        paquet.taille_buffer = taille;
        paquet.numero_paquet = compteur++;
        memcpy(paquet.buffer, resultat + offset, taille);
        rep.paquet = paquet;
        rep.reponse = ENVOIE_FICHIER;
        Rio_writen(connfd, &rep, sizeof(reponse_t));
        offset += taille;
    }
}