#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>


#define BUFSZ 501

void usage(int argc, char **argv)
{

    printf("usage: %s <v4|v6> <server port>zn", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);

    exit(EXIT_FAILURE);
}
void remove_directory(const char* path) {
    DIR* dir = opendir(path);
    struct dirent* entry;

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            char file_path[BUFSZ];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            snprintf(file_path, BUFSZ, "%s/%s", path, entry->d_name);
            remove(file_path);
        }
        closedir(dir);
    }

    remove(path);
}


char * extrairFilename (const char * mensagem){
    const char *linha_init = strchr(mensagem, '\n');
    if (linha_init == NULL)
    {
        printf("Caractere de nova linha não encontrado na mensagem.\n");
        return NULL;
    }

    size_t tam_filename = linha_init - mensagem;
    char *filename = (char *)malloc(tam_filename + 1);
    strncpy(filename, mensagem, tam_filename);
    filename[tam_filename] = '\0';

    return filename;
}

void createFile(const char *mensagem)
{

    const char *filename = extrairFilename(mensagem);

    char *path = (char *)malloc(strlen("serverFiles/") + strlen(filename) + 1);

    sprintf(path, "serverFiles/%s", filename);

    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        printf("Não foi possível criar o arquivo.\n");
        return;
    }

    fputs(mensagem, file);
    fclose(file);
    free (path);

    return;
}

int verificarOverwritting(const char* filename) {
    char path[BUFSZ];
    sprintf(path, "serverFiles/%s", filename);

    struct stat st;
    if (stat(path, &st) == 0) {
        // File exists
        return 1;
    } else {
        // File does not exist
        return 0;
    }
}


int main(int argc, char **argv)
{

    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsocket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // bind
    if (0 != bind(s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    int break_server = 0;
    char files_saved[BUFSZ][BUFSZ];
    int num_files = 0;

    const char* folderName = "serverFiles";

    //remove a pasta que tiver
    remove_directory(folderName);
    // Cria a nova pasta com permissões padrão (0777)
    mkdir(folderName, 0777);

    while (1)
    {
        // listen
        if (0 != listen(s, 10))
        {
            logexit("listen");
        }

        char addrstr[BUFSZ];
        addrtostr(addr, addrstr, BUFSZ);
        printf("bound to %s, waiting conections \n", addrstr);

        // accept
        // c = cliente (cstorage = client_storage)

        struct sockaddr_storage cstorage;
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, (struct sockaddr *)&cstorage, &caddrlen);

        if (csock == -1)
        {
            logexit("accept");
        }
        else
        {
            while (1)
            {

                char caddrstr[BUFSZ];
                addrtostr((struct sockaddr *)&cstorage, caddrstr, BUFSZ);
                printf("[log] Connection from %s \n", caddrstr);


                char buf[BUFSZ];
                ssize_t count = recv(csock, buf, BUFSZ, 0);
                if (count == -1)
                {
                    logexit("send");
                }

                // Verificar se o cliente solicitou o encerramento da sessão
                if (strncmp(buf, "exit", 4) == 0)
                {
                    // Enviar mensagem de confirmação de encerramento ao cliente
                    const char *exit_confirmation = "connection closed";
                    ssize_t sent = send(csock, exit_confirmation, strlen(exit_confirmation), 0);
                    if (sent == -1)
                    {
                        printf("Error sending exit confirmation\n");
                    }
                    break_server = 1;
                    printf("Connection closed\n");
                    break; // Encerra o loop principal do servidor
                }

                // Verificar se foi comando inválido
                if (strncmp(buf, "invalido", strlen("invalido")) == 0)
                {

                    printf("Connection closed\n");
                    break; // Encerra o loop principal do servidor
                }


                char* filename = extrairFilename(buf);

                if(verificarOverwritting(filename)){
                    printf("file %s overwritten\n", filename);
                    const char *confirmation = "This file already exists";
                    send(csock, confirmation, strlen(confirmation) + 1, 0);
                    continue;
                }
            
                //Adicionar novo arquivo na lista
                strcpy(files_saved[num_files], filename);
                num_files +=1;

                //Verifica se recebeu corretamente
                if (count <= 0) {
                    printf("Error receiving file %s\n", filename);
                    exit(EXIT_FAILURE);
                }

                if (count > 0){
                    printf("file %s received\n", filename);
                }

                //CRIAR O ARQUIVO
                createFile(buf);

                // CONFIRMAR O RECEBIMENTO DA MENSAGEM PARA O CLIENTE
                const char *confirmation = "Message received";
                ssize_t sent = send(csock, confirmation, strlen(confirmation) + 1, 0);
                if (sent == -1)
                {
                    // Ocorreu um erro ao enviar a confirmação ao cliente
                    printf("Error sending confirmation\n");
                }
                else if (sent < strlen(confirmation))
                {
                    // Nem todos os dados da confirmação foram enviados
                    printf("Incomplete confirmation sent to the client\n");
                }
                else
                {
                    printf("Confirmation sent to the client\n");
                }

                memset(buf,0,BUFSZ);
            }
            close(csock);
        }
        if (break_server == 1)
        {
            break;
        }
    }   
    
    exit(EXIT_SUCCESS);
}