#ifndef STRUCTURES_H

#define STRUCTURES_H

#include "csapp.h"

#define PORT 2121
#define MAX_NAME_LEN 256
#define MAX_PAQ_LEN 1024

// Structure de la requete
typedef enum { 
    GET = 0,
    PUT = 1,
    LS = 3
} typereq_t;
 
typedef struct {
    typereq_t type;
    char nomFichier[MAXLINE];
    int fd_cible;
    int num_paquet;
} request_t;

// Structure de la reponse
typedef enum{
    ACK = 0,
    ENVOIE_FICHIER = 1,
    ENVOIE_TERMINER = 2,
    ERREUR_SERVEUR = 10,
    ERREUR_REQUETE_INVALIDE = 11,
    ERREUR_FICHIER_INACCESSIBLE = 51,
    ERREUR_FICHIER_INEXISTANT = 52,
} typerep_t;

typedef struct {
    char buffer[MAX_PAQ_LEN];
    int taille_buffer;
    int numero_paquet;
} paquet_t;

typedef struct{
    typerep_t reponse;
    int nb_paquets;
    paquet_t paquet;
} reponse_t;


#endif