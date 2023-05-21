#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


#define BUFSZ 501


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
    
    int break_server = 0;
    char files_saved [BUFSZ][BUFSZ];
    int num_files = 0;

    while(1){
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
                addrtostr((struct sockaddr *)&cstorage, caddrstr, BUFSZ);
                printf("[log] Connection from %s \n", caddrstr);

                // Receber o nome do arquivo
                char filename[BUFSZ];
                memset(filename,0,BUFSZ);

                ssize_t filename_len = recv(csock, filename, BUFSZ - 1, 0);

                //Verificar se arquivo já existe
                for (int i = 0; i < num_files; i++){
                    if (0 == strcmp(filename, files_saved[i])){
                        printf("file %s overwritten\n", filename);
                        break_server = 1;
                        break;
                    }
                }
                if(break_server){
                    break;
                }
                //Adicionar novo arquivo na lista
                strcpy(files_saved[num_files], filename);
                num_files +=1;

                //Verificar se o cliente solicitou o encerramento da sessão
                if (strncmp(filename, "exit", 4) == 0) {
                    // Enviar mensagem de confirmação de encerramento ao cliente
                    const char *exit_confirmation = "connection closed";
                    ssize_t sent = send(csock, exit_confirmation, strlen(exit_confirmation), 0);
                    if (sent == -1) {
                        printf("Error sending exit confirmation\n");
                    }
                    break_server = 1;
                    printf("Connection closed\n");
                    break; // Encerra o loop principal do servidor
                }

                //Verificar se foi comando inválido
                if (strncmp(filename, "invalido", strlen("invalido")) == 0) {

                    printf("Connection closed\n");
                    break; // Encerra o loop principal do servidor
                }
                //Verifica se recebeu corretamente
                if (filename_len <= 0) {
                    printf("Error receiving file %s\n", filename);
                    exit(EXIT_FAILURE);
                }

                if (filename_len > 0){
                    printf("file %s received\n", filename);
                }

                
                // CONFIRMAR O RECEBIMENTO DA MENSAGEM PARA O CLIENTE
                const char *confirmation = "Message received";
                ssize_t sent = send(csock, confirmation, strlen(confirmation) + 1, 0);
                if (sent == -1) {
                    // Ocorreu um erro ao enviar a confirmação ao cliente
                    printf("Error sending confirmation\n");
                } else if (sent < strlen(confirmation)) {
                    // Nem todos os dados da confirmação foram enviados
                    printf("Incomplete confirmation sent to the client\n");
                } else {
                    printf("Confirmation sent to the client\n");
                }       
            }
            close(csock);   
        }
        if (break_server == 1){
            break;
        }
    }

    exit(EXIT_SUCCESS);
}
