#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_COMMAND_SIZE 1024
#define MAX_RESPONSE_SIZE 4096

void handleCommand(int clientSocket, const char* command) {
    char response[MAX_RESPONSE_SIZE];

    if (strcmp(command, "cd") == 0) {
        printf("Ingrese el directorio al que desea cambiar en el agente: ");
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = '\0';
    }
    else if (strcmp(command, "pwd") == 0) {
        strcpy(response, "pwd");
    }
    else if (strcmp(command, "ls") == 0) {
        strcpy(response, "ls");
    }
    else if (strncmp(command, "put", 3) == 0) {
        printf("Ingrese el nombre del archivo a enviar al agente: ");
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = '\0';
    }
    else if (strncmp(command, "get", 3) == 0) {
        printf("Ingrese el nombre del archivo a recibir del agente: ");
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = '\0';
    }
    else if (strncmp(command, "exec", 4) == 0) {
        printf("Ingrese el comando de PowerShell a ejecutar en el agente: ");
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = '\0';
    }
    else {
        strcpy(response, "Comando no reconocido");
    }

    // Enviar la respuesta al agente
    if (send(clientSocket, response, strlen(response), 0) == -1) {
        perror("Error al enviar la respuesta al agente");
        return;
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;
    char command[MAX_COMMAND_SIZE];

    // Crear socket del servidor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket del servidor");
        return 1;
    }

    // Configurar la estructura de la dirección del servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serverAddr.sin_zero), '\0', 8);

    // Vincular el socket a la dirección y puerto especificados
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1) {
        perror("Error al vincular el socket del servidor");
        return 1;
    }

    // Escuchar conexiones entrantes
    if (listen(serverSocket, 1) == -1) {
        perror("Error al escuchar en el socket del servidor");
        return 1;
    }

    printf("Servidor escuchando en el puerto 12345...\n");

    // Aceptar la conexión del agente
    clientAddrLen = sizeof(struct sockaddr_in);
    if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen)) == -1) {
        perror("Error al aceptar la conexión del agente");
        return 1;
    }

    printf("Agente conectado al servidor\n");

    while (1) {
        printf("Ingrese un comando para enviar al agente: ");
        fgets(command, sizeof(command), stdin);

        // Eliminar el salto de línea final
        command[strcspn(command, "\n")] = '\0';

        // Enviar el comando al agente y manejar la respuesta
        handleCommand(clientSocket, command);
    }

    // Cerrar sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}
