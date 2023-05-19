#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 501


void usage(int argc, char **argv){

    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int valida_extensao (char extensao[5]){
        
        // Lista de extensões válidas
        const char *extensoes_validas[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
        int num_extensoes_validas = sizeof(extensoes_validas) / sizeof(extensoes_validas[0]);

        for (int i = 0; i < num_extensoes_validas; i++) {
            if (strcmp(extensao, extensoes_validas[i]) == 0) {
                return 1;
                break;
            }
        }
    return 0;        
}


int main(int argc, char **argv){

    if (argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    if (s == -1){
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    if (0 != connect(s, addr, sizeof(storage))){
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("connected to %s \n", addrstr);

    FILE *file = NULL;
    char filename[BUFSZ] = "";

    while(1){
        
        //Comando
        printf("Comando: ");
        char comando[BUFSZ];
        fgets(comando, BUFSZ - 1, stdin);

        if (strncmp(comando, "select file ", strlen("select file ")) == 0) {

            if (file != NULL) {
                fclose(file);
            }           
            
            char buf[BUFSZ] = "";

            sscanf(comando, "select file %[^\n]", buf);
            // Abrindo o arquivo para leitura em modo binário
            file = fopen(buf, "r"); 

            char *extensao = strrchr(buf, '.');

            if (extensao == NULL || !valida_extensao(extensao))
            {
                printf("%s not valid\n", buf);

            }
            else if (file == NULL ){
                printf("%s does not exist\n", buf);
                exit(EXIT_FAILURE);
            }
            else{
                strcpy(filename, buf);
                file = fopen(filename, "rb"); 
                
                if(file == NULL){
                    logexit("fopen");
                    exit(EXIT_FAILURE);
                }

                printf("%s selected\n", filename);

            }
                 
        } 
        else if (strncmp(comando, "send file", strlen("send file")) == 0) {
           
            if(file == NULL){
                printf("no file selected!\n");
                continue;
            }

            // Enviar o nome do arquivo para o servidor
            size_t bytes_sent  = send(s, filename, strlen(filename) + 1, 0); // Qual o nº de bites efetivamente transmitidos

            if (bytes_sent == -1) {
                logexit("send");
            }
            
            // RECEBER A CONFIRMAÇÃO DO SERVIDOR
            char recv_buf[BUFSZ];
            memset(recv_buf, 0, BUFSZ);
            ssize_t count = recv(s, recv_buf, BUFSZ - 1, 0);
            if (count == -1) {
                logexit("recv");
            } else if (count == 0) {
                // O servidor fechou a conexão
                printf("Connection closed by the server\n");
            } else {
                printf("Server confirmation: %s\n", recv_buf);
            }

        }        
        else if (strncmp(comando, "exit", 4) == 0) {// ENCERRAR CONEXÃO
            // Envia mensagem especial para o servidor indicando o encerramento da sessão
            const char *exit_message = "exit";
            ssize_t sent = send(s, exit_message, strlen(exit_message), 0);
            if (sent == -1) {
                logexit("send");
            }
            
            printf("Session terminated\n");
            break; // Encerra o loop principal do cliente
        }
        else{
            // Envia mensagem especial para o servidor indicando o comando errado
            const char *invalido = "invalido";
            ssize_t sent = send(s, invalido, strlen(invalido), 0);
            if (sent == -1) {
                logexit("send");
            }
            break; // Encerra o loop principal do cliente
        }
 
    } 
    if (file != NULL) {
        fclose(file);
    }
    close(s);

}
