#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>


#define BUFSZ 500

void usage(int argc, char **argv){

    printf("usage: %s <v4|v6> <server port>zn", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);

    exit(EXIT_FAILURE);

}

int main(int argc, char **argv){

    if (argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1){
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("setsocket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    //bind
    if (0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
    }
    
    //listen
    if (0 != listen(s, 10)){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);    
    printf("bound to %s, waiting conections \n", addrstr);

    //accept
    //c = cliente (cstorage = client_storage)
    
    struct sockaddr_storage cstorage;
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, (struct sockaddr *)&cstorage, &caddrlen);
    if (csock == -1){
        logexit("accept");
    }
    else{
        while(1){

            char caddrstr[BUFSZ];
            if (getnameinfo((struct sockaddr *)&cstorage, caddrlen, caddrstr, BUFSZ, NULL, 0, NI_NUMERICHOST) != 0) {
                logexit("getnameinfo");
            }
            printf("[log] Connection from %s \n", caddrstr);

            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            ssize_t count = recv(csock, buf, BUFSZ-1, 0);

            printf("[msg] %s, %zd bytes: %s\n", caddrstr, count, buf);

            // Verificar se a mensagem toda chegou
            if (count > 0) {
                // A mensagem foi recebida com sucesso
                printf("[log] File received and stored successfully.\n");
            } 

            // Confirmar o recebimento da mensagem ao cliente
            const char *confirmation = "Message received";
            ssize_t sent = send(csock, confirmation, strlen(confirmation), 0);
            if (sent == -1) {
                // Ocorreu um erro ao enviar a confirmação ao cliente
                printf("Error sending confirmation\n");
            } else {
                printf("Confirmation sent to the client\n");
            }


        }
    }
    

    exit(EXIT_SUCCESS);
}