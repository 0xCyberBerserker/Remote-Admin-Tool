#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define MAX_BUFFER_SIZE 1024
#define MAX_RECONNECT_ATTEMPTS 20
#define RECONNECT_INTERVAL 10

bool IsConnectionActive(SOCKET socket) {
    // Aquí implementa la lógica para chequear si la conexión está activa
    // Puedes utilizar una llamada a una función o método específico según tu caso

    // Ejemplo:
    // int error = 0;
    // socklen_t len = sizeof(error);
    // if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0) {
    //     return false;
    // }
    // return error == 0;

    // En este ejemplo, se verifica si hay algún error en el socket

    int error = 0;
    int len = sizeof(error);
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0) {
        perror("Error al obtener el estado del socket");
        exit(1);
    }

    return error == 0;
}

int main()
{
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    int clientAddressLength;
    char buffer[MAX_BUFFER_SIZE];
    int reconnectAttempts = 0;

    // Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Error al inicializar Winsock");
        exit(1);
    }

    // Crear el socket del servidor
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        perror("Error al crear el socket del servidor");
        exit(1);
    }

    // Configurar la estructura de dirección del servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Vincular el socket a la dirección del servidor
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        perror("Error al vincular el socket al servidor");
        exit(1);
    }

    // Poner el socket del servidor en modo de escucha
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        perror("Error al poner el socket en modo de escucha");
        exit(1);
    }

    printf("Servidor iniciado. Esperando conexiones...\n");

    while (1) {
        // Aceptar la conexión entrante del cliente
        clientAddressLength = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            perror("Error al aceptar la conexión del cliente");
            exit(1);
        }

        printf("Cliente conectado.\n");

        while (1) {
            // Leer los datos enviados por el cliente
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead == SOCKET_ERROR) {
                perror("Error al leer los datos del cliente");
                exit(1);
            } else if (bytesRead == 0) {
                printf("El cliente ha cerrado la conexión.\n");
                break;
            }

            printf("Mensaje recibido del cliente: %s\n", buffer);

            // Enviar una respuesta al cliente
            const char* response = "Mensaje recibido por el servidor";
            if (send(clientSocket, response, strlen(response), 0) == SOCKET_ERROR) {
                perror("Error al enviar la respuesta al cliente");
                exit(1);
            }

            // Comprobar si la conexión sigue activa
            if (!IsConnectionActive(clientSocket)) {
                printf("La conexión se ha perdido. Intentando reconectar...\n");

                // Cerrar el socket actual
                closesocket(clientSocket);

                // Intentar reconectar
                reconnectAttempts++;
                if (reconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
                    printf("Se ha superado el número máximo de intentos de reconexión. Saliendo...\n");
                    break;
                }

                printf("Intento de reconexión número %d\n", reconnectAttempts);
                Sleep(RECONNECT_INTERVAL * 1000);

                // Volver a aceptar la conexión entrante del cliente
                clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
                if (clientSocket == INVALID_SOCKET) {
                    perror("Error al aceptar la conexión del cliente");
                    exit(1);
                }

                printf("Cliente reconectado.\n");
            }
        }

        // Cerrar el socket del cliente
        closesocket(clientSocket);
    }

    // Cerrar el socket del servidor
    closesocket(serverSocket);

    // Liberar Winsock
    WSACleanup();

    return 0;
}
