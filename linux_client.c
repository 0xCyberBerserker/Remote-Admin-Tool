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

int sendCommand(int socket, const char* command) {
    if (send(socket, command, strlen(command), 0) < 0) {
        printf("Error al enviar el comando\n");
        return -1;
    }
    return 0;
}

int receiveResponse(int socket, char* response) {
 memset(response, 0, MAX_RESPONSE_SIZE);
    if (recv(socket, response, MAX_RESPONSE_SIZE - 1, 0) < 0) {
        printf("Error al recibir la respuesta\n");
        return -1;
    }
    return 0;
}




int receiveFile(int socket, const char* fileName) {
    FILE* file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("Error al abrir el archivo para escritura\n");
        return -1;
    }

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    do {
        bytesRead = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            printf("Error al recibir el archivo del servidor\n");
            fclose(file);
            return -1;
        }
        fwrite(buffer, 1, bytesRead, file);
    } while (bytesRead == sizeof(buffer));

    fclose(file);
    return 0;
}

int sendFile(int socket, const char* fileName) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Error al abrir el archivo para lectura\n");
        return -1;
    }

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    do {
        bytesRead = fread(buffer, 1, sizeof(buffer), file);
        if (bytesRead > 0) {
            if (send(socket, buffer, bytesRead, 0) < 0) {
                printf("Error al enviar el archivo al servidor\n");
                fclose(file);
                return -1;
            }
        }
    } while (bytesRead > 0);

    fclose(file);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ip_servidor>\n", argv[0]);
        return 1;
    }

    char* serverIP = argv[1];
    int serverPort = 12345;

    int clientSocket;
    struct sockaddr_in serverAddr;

    // Crear socket del cliente
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error al crear el socket del cliente\n");
        return 1;
    }

    // Configurar la estructura de la dirección del servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
        printf("Dirección IP inválida\n");
        return 1;
    }

    // Conectar al servidor
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error al conectar al servidor\n");
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
        }
        else if (strcmp(command, "pwd") == 0) {
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
        }
        // Enviar el comando al servidor
        if (sendCommand(clientSocket, command) != 0) {
            continue;
        }

        // Recibir la respuesta del servidor
        if (receiveResponse(clientSocket, response) != 0) {
            continue;
        }

        // Interpretar la respuesta del servidor
        if (strcmp(response, "READY_FOR_UPLOAD") == 0) {
            // Enviar el archivo al servidor
            char fileName[MAX_FILE_NAME];
            sscanf(command, "put %s", fileName);
            if (sendFile(clientSocket, fileName) == -1) {
                printf("Error al enviar el archivo al servidor\n");
            } else {
                printf("Archivo enviado exitosamente\n");
            }
        } else if (strcmp(response, "READY_FOR_DOWNLOAD") == 0) {
            // Recibir el archivo del servidor
            char fileName[MAX_FILE_NAME];
            sscanf(command, "get %s", fileName);
            if (receiveFile(clientSocket, fileName) == -1) {
                printf("Error al recibir el archivo del servidor\n");
            } else {
                printf("Archivo recibido exitosamente\n");
            }
        } else {
            // Mostrar la respuesta del servidor
            printf("Respuesta del servidor: %s\n", response);
        }
    }

    // Cerrar socket del cliente
    close(clientSocket);

    return 0;
}
