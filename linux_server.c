#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_COMMAND_SIZE 1024
#define MAX_RESPONSE_SIZE 4096
#define MAX_FILE_NAME 256
#define MAX_BUFFER_SIZE 4096

void showHelp() {
    printf("Comandos disponibles:\n");
    printf("    cd <directorio>          - Cambiar de directorio en el servidor\n");
    printf("    pwd                      - Mostrar el directorio actual en el servidor\n");
    printf("    ls                       - Listar archivos y directorios en el servidor\n");
    printf("    put <archivo_local>      - Subir un archivo al servidor\n");
    printf("    get <archivo_remoto>     - Descargar un archivo del servidor\n");
    printf("    exec <comando_powershell> - Ejecutar un comando de PowerShell en el servidor\n");
    printf("    exit                     - Salir del cliente\n");
}

int sendResponse(int socket, const char* response) {
    if (send(socket, response, strlen(response), 0) < 0) {
        printf("Error al enviar la respuesta\n");
        return -1;
    }
    return 0;
}

int receiveCommand(int socket, char* command) {
    memset(command, 0, MAX_COMMAND_SIZE);
    if (recv(socket, command, MAX_COMMAND_SIZE - 1, 0) < 0) {
        printf("Error al recibir el comando\n");
        return -1;
    }
    return 0;
}

int receiveFile(int clientSocket, const char* fileName) {
    FILE* file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("Error al crear el archivo %s\n", fileName);
        return -1;
    }

    // Recibir el tamaño del archivo del cliente
    long fileSize;
    if (recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0) < 0) {
        printf("Error al recibir el tamaño del archivo del cliente\n");
        fclose(file);
        return -1;
    }

    // Recibir y escribir los datos del archivo en bloques
    char buffer[MAX_BUFFER_SIZE];
    long bytesReceived = 0;
    size_t bytesRead;
    while (bytesReceived < fileSize) {
        int remainingBytes = fileSize - bytesReceived;
        int bytesToReceive = (remainingBytes > MAX_BUFFER_SIZE) ? MAX_BUFFER_SIZE : remainingBytes;
        bytesRead = recv(clientSocket, buffer, bytesToReceive, 0);
        if (bytesRead <= 0) {
            printf("Error al recibir datos del cliente\n");
            fclose(file);
            return -1;
        }
        fwrite(buffer, 1, bytesRead, file);
        bytesReceived += bytesRead;
    }

    fclose(file);
    return 0;
}

int sendFile(int clientSocket, const char* fileName) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Error al abrir el archivo %s\n", fileName);
        return -1;
    }

    // Obtener el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Enviar el tamaño del archivo al cliente
    if (send(clientSocket, (const char*)&fileSize, sizeof(fileSize), 0) < 0) {
        printf("Error al enviar el tamaño del archivo al cliente\n");
        fclose(file);
        return -1;
    }

    // Enviar el contenido del archivo al cliente en bloques
    char buffer[MAX_BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) < 0) {
            printf("Error al enviar el archivo al cliente\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int agentPort = 12345;

    // Crear socket del servidor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error al crear el socket del servidor\n");
        return 1;
    }

    // Configurar la estructura de la dirección del servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(agentPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Vincular el socket del servidor a la dirección y puerto especificados
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error al vincular el socket del servidor\n");
        return 1;
    }

    // Escuchar conexiones entrantes
    if (listen(serverSocket, 1) < 0) {
        printf("Error al escuchar conexiones entrantes\n");
        return 1;
    }

    printf("Servidor esperando conexiones en el puerto %d/TCP\n", agentPort);

    while (1) {
        // Aceptar la conexión entrante
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen)) < 0) {
            printf("Error al aceptar la conexión entrante\n");
            continue;
        }

        printf("Cliente conectado desde %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        char command[MAX_COMMAND_SIZE];
        char response[MAX_RESPONSE_SIZE];

        showHelp();

        while (1) {
            // Recibir el comando del cliente
            if (receiveCommand(clientSocket, command) != 0) {
                break;
            }

            // Comprobar el comando recibido
            if (strcmp(command, "exit") == 0) {
                printf("Cliente desconectado desde %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                break;
            }
            else if (strcmp(command, "pwd") == 0) {
                // Obtener el directorio actual del servidor
                char currentDir[MAX_RESPONSE_SIZE];
                if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
                    printf("Error al obtener el directorio actual del servidor\n");
                    break;
                }

                // Enviar la respuesta al cliente
                if (sendResponse(clientSocket, currentDir) != 0) {
                    break;
                }
            }
            else if (strncmp(command, "get ", 4) == 0) {
                // Obtener el nombre del archivo remoto
                char fileName[MAX_FILE_NAME];
                sscanf(command, "get %s", fileName);

                // Enviar la confirmación al cliente
                if (sendResponse(clientSocket, "READY_FOR_DOWNLOAD") != 0) {
                    break;
                }

                // Recibir el archivo del cliente
                if (receiveFile(clientSocket, fileName) == -1) {
                    printf("Error al recibir el archivo del cliente\n");
                } else {
                    printf("Archivo recibido exitosamente: %s\n", fileName);
                }
            }
            else if (strncmp(command, "put ", 4) == 0) {
                // Obtener el nombre del archivo local
                char fileName[MAX_FILE_NAME];
                sscanf(command, "put %s", fileName);

                // Enviar la confirmación al cliente
                if (sendResponse(clientSocket, "READY_FOR_UPLOAD") != 0) {
                    break;
                }

                // Enviar el archivo al cliente
                if (sendFile(clientSocket, fileName) == -1) {
                    printf("Error al enviar el archivo al cliente\n");
                } else {
                    printf("Archivo enviado exitosamente: %s\n", fileName);
                }
            }
            else {
                // Comando no reconocido, enviar respuesta genérica
                if (sendResponse(clientSocket, "Comando no reconocido") != 0) {
                    break;
                }
            }
        }

        // Cerrar el socket del cliente
        close(clientSocket);
    }

    // Cerrar el socket del servidor
    close(serverSocket);

    return 0;
}
