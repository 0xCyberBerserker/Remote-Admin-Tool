#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

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

int sendCommand(SOCKET socket, const char* command) {
    if (send(socket, command, strlen(command), 0) == SOCKET_ERROR) {
        printf("Error al enviar el comando\n");
        return -1;
    }
    return 0;
}

int receiveResponse(SOCKET socket, char* response) {
    memset(response, 0, MAX_RESPONSE_SIZE);
    if (recv(socket, response, MAX_RESPONSE_SIZE - 1, 0) == SOCKET_ERROR) {
        printf("Error al recibir la respuesta\n");
        return -1;
    }
    return 0;
}

int receiveFile(SOCKET serverSocket, const char* fileName) {
    FILE* file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("Error al crear el archivo %s\n", fileName);
        return -1;
    }

    // Recibir el tamaño del archivo del servidor
    long fileSize;
    if (recv(serverSocket, (char*)&fileSize, sizeof(fileSize), 0) == SOCKET_ERROR) {
        printf("Error al recibir el tamaño del archivo del servidor\n");
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
        bytesRead = recv(serverSocket, buffer, bytesToReceive, 0);
        if (bytesRead <= 0) {
            printf("Error al recibir datos del servidor\n");
            fclose(file);
            return -1;
        }
        fwrite(buffer, 1, bytesRead, file);
        bytesReceived += bytesRead;
    }

    fclose(file);
    return 0;
}

int sendFile(SOCKET clientSocket, const char* fileName) {
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
    if (send(clientSocket, (const char*)&fileSize, sizeof(fileSize), 0) == SOCKET_ERROR) {
        printf("Error al enviar el tamaño del archivo al cliente\n");
        fclose(file);
        return -1;
    }

    // Enviar el contenido del archivo al cliente en bloques
    char buffer[MAX_BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
            printf("Error al enviar el archivo al cliente\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error al inicializar Winsock\n");
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Error al crear el socket del cliente\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("192.168.1.46"); // Cambia la dirección IP al servidor
    serverAddr.sin_port = htons(12345); // Cambia el puerto si es necesario

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Error al conectar al servidor\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char command[MAX_COMMAND_SIZE];
    char response[MAX_RESPONSE_SIZE];

    printf("Cliente conectado al servidor\n");
    showHelp();

    while (1) {
        printf("Ingrese un comando: ");
        fgets(command, sizeof(command), stdin);

        // Eliminar el salto de línea final
        command[strcspn(command, "\n")] = 0;

        // Comprobar el comando ingresado
        if (strcmp(command, "--help") == 0) {
            showHelp();
            continue;
        } else if (strcmp(command, "exit") == 0) {
            printf("Saliendo del cliente\n");
            break;
        } else if (strcmp(command, "pwd") == 0) {
            // Enviar el comando al servidor
            if (sendCommand(clientSocket, command) != 0) {
                continue;
            }

            // Recibir la respuesta del servidor
            if (receiveResponse(clientSocket, response) != 0) {
                continue;
            }

            // Mostrar la respuesta del servidor
            printf("Directorio actual en el servidor: %s\n", response);
        } else if (strncmp(command, "get ", 4) == 0) {
            // Enviar el comando al servidor
            if (sendCommand(clientSocket, command) != 0) {
                continue;
            }

            // Recibir la respuesta del servidor
            if (receiveResponse(clientSocket, response) != 0) {
                continue;
            }

            if (strcmp(response, "READY_FOR_DOWNLOAD") == 0) {
                // Recibir el archivo del servidor
                char fileName[MAX_FILE_NAME];
                sscanf(command, "get %s", fileName);
                if (receiveFile(clientSocket, fileName) == -1) {
                    printf("Error al recibir el archivo del servidor\n");
                } else {
                    printf("Archivo descargado: %s\n", fileName);
                }
            } else {
                printf("Error al descargar el archivo del servidor: %s\n", response);
            }
        } else if (strncmp(command, "put ", 4) == 0) {
            // Enviar el comando al servidor
            if (sendCommand(clientSocket, command) != 0) {
                continue;
            }

            // Recibir la respuesta del servidor
            if (receiveResponse(clientSocket, response) != 0) {
                continue;
            }

            if (strcmp(response, "READY_FOR_UPLOAD") == 0) {
                // Enviar el archivo al servidor
                char fileName[MAX_FILE_NAME];
                sscanf(command, "put %s", fileName);
                if (sendFile(clientSocket, fileName) == -1) {
                    printf("Error al enviar el archivo al servidor\n");
                } else {
                    printf("Archivo enviado: %s\n", fileName);
                }
            } else {
                printf("Error al subir el archivo al servidor: %s\n", response);
            }
        } else {
            // Enviar el comando al servidor
            if (sendCommand(clientSocket, command) != 0) {
                continue;
            }

            // Recibir la respuesta del servidor
            if (receiveResponse(clientSocket, response) != 0) {
                continue;
            }

            // Mostrar la respuesta del servidor
            printf("%s\n", response);
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
