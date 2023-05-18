#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>


void usage(int argc, char **argv){

    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 500

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

    while(1){
        
        //Comando
        char comando[BUFSIZ];
        fgets(comando, BUFSIZ - 1, stdin);

        FILE *file;
        
        char str[] = "select file ";

        if (strncmp(comando, str, strlen(str)) == 0) {

            char *fileStart = strstr(comando, str) + strlen(str);
            char filename[BUFSZ];
            sscanf(fileStart, "%s", filename);
            
            //Extrai o nome do arquivo
            size_t filename_len = strlen(filename);
            if (filename[filename_len - 1] == '\n'){
                filename[filename_len - 1] = '\0';
            }
                
            // Abrindo o arquivo para leitura em modo binário
            file = fopen(filename, "rb"); 

            if (file == NULL)
            {
                printf("%s does not exist\n", filename);
                exit(EXIT_FAILURE);

            }
            else{
                printf("%s selected\n", filename);
            }
            
        } 

        if (strncmp(comando, "send file", 9) == 0) {
            
            // Lendo o conteúdo do arquivo
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *file_buffer = (char *)malloc(file_size);
            if (file_buffer == NULL)
            {
                logexit("malloc");
            }

            size_t bytes_read = fread(file_buffer, sizeof(char), file_size, file);
            if (bytes_read != file_size)
            {
                logexit("fread");
            }
            
            // Enviando o conteúdo do arquivo para o servidor
            size_t total_sent = 0;
            while (total_sent < file_size)
            {
                ssize_t bytes_sent = send(s, file_buffer + total_sent, file_size - total_sent, 0);
                if (bytes_sent == -1)
                {
                    logexit("send");
                }
                total_sent += bytes_sent;
            }

            free(file_buffer);
            fclose(file);
            
            // Recebendo a confirmação de recebimento do servidor
            char recv_buf[BUFSZ];
            memset(recv_buf, 0, BUFSZ);

            ssize_t count = recv(s, recv_buf, BUFSZ - 1, 0);
            if (count == -1)
            {
                logexit("recv");
            }

            printf("Server confirmation: %s\n", recv_buf);
        }


        if(strncmp(comando, "exit", 4) == 0){
            close(s);
            exit(EXIT_SUCCESS); 
        }
        
        /*
        char close_msg[] = "close";
        count = send(s, close_msg, strlen(close_msg) + 1, 0);
        if (count != strlen(close_msg) + 1)
        {
            logexit("send");
        }
        }
    */
        
    }   
}