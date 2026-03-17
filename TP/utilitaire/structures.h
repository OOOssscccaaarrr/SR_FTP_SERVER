#ifndef STRUCTURES_H

#define STRUCTURES_H


#define PORT 2121

typedef enum { 
    GET = 0,
    PUT = 1,
    LS = 3
} typereq_t;
 
typedef struct {
    typereq_t type;
    char nomfichier[MAXLINE];
    int ptr_fichier;
} request_t;

#define PORT 2121

typedef enum{
    ACK = 0,
    ENVOIE_FICHIER = 1,
    ERREUR_SERVEUR = 10,
    ERREUR_REQUETE_INVALIDE = 11,
    ERREUR_FICHIER_INACCESSIBLE = 51,
    ERREUR_FICHIER_INEXISTANT = 52,
} typerep_t;

typedef struct{
    typerep_t reponse;
    char nomFichier[MAX_NAME_LEN];
} reponse_t;


#endif