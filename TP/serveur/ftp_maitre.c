#include <ftp_maitre.h>


void main(){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];

    serveur_escalve_t tbl_escalve[NB_SLAVE];
    request_t req;
    reponse_t rep;

    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT_DEFAUT);

    for (int i = 0 ; i < NB_SLAVE; i++){
        tbl_escalve[i].ip = "127.0.0.1";
        tbl_escalve[i].port = PORT_DEFAUT + i + 1;
        system("./ftp_serveur"); //TODO ARGUEMNT
    }

    while(1){

        Getnameinfo((SA *) &clientaddr, clientlen,
                        client_hostname, MAX_NAME_LEN, 0, 0, 0);
            
        /* determine the textual representation of the client's IP address */
        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                INET_ADDRSTRLEN);



    }


}