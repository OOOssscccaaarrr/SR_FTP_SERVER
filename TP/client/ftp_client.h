#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include "../utilitaire/structures.h"

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
void cmd_get(rio_t *rio, request_t req, int clientfd);

/**
 * Envoie une requête de fermeture au serveur et attend sa confirmation (ACK).
 * @param rio : buffer de lecture rio
 * @param req : requête à envoyer
 * @param clientfd : file descriptor de la connexion
 */
void cmd_ferme(rio_t *rio, request_t req, int clientfd);

/**
 * Envoie une requête LS au serveur avec les arguments dans nomFichier.
 * Reçoit le résultat paquet par paquet et l'affiche directement sur stdout.
 * @param rio : buffer de lecture rio
 * @param req : requête à envoyer (contient les arguments ls dans nomFichier)
 * @param clientfd : file descriptor de la connexion
 */
void cmd_ls(rio_t *rio, request_t req, int clientfd);

/**
 * Envoie une requête de redirection au serveur maître.
 * Retourne les informations de connexion de l'esclave assigné.
 * @param rio : buffer de lecture rio
 * @param clientfd : file descriptor de la connexion au maître
 * @return serveur_esclave_t contenant l'ip et le port de l'esclave, port=0 en cas d'échec
 */
serveur_esclave_t cmd_connexion(rio_t *rio, int clientfd);


#endif