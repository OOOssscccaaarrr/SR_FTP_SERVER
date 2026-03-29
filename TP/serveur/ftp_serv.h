#ifndef FTP_SERV_H
#define FTP_SERV_H

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
void afficher_message(int numero_serveur, char* client_hostname, char* message, char* argument);

/**
 * Envoie une réponse d'erreur au client.
 * @param connfd : file descriptor de la connexion client
 * @param type   : type d'erreur à envoyer
 */
void reponse_err(int connfd, typerep_t type);

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
void traitement_get(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]);

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
void traitement_ls(rio_t *rio, int connfd, request_t req, int numero_serv, char client_hostname[MAX_NAME_LEN]);

#endif