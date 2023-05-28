This software is intended to be a Remote Administration Tool developed in C for a Windows Server and a Linux Client.
It is intended to have some personalized commands and execute commands on powershell by the argument personalized command exec and then the powershell command you want to execute.

Usable commands:

    cd directory             

    pwd                         - Shows the full path of the directory we are in

    ls                          - Just like a Dir or an ls on a Linux Machine

    put local_file              - Upload a file to the server

    get remote_file             - Download a file from the server

    exec Powershell_Command     - Execute a powershell command on the server

    exit                        - Exits the client


**Compilation:**

You need to compile the server on a Windows machine and need MinGW and GCC installed.

Latest compiled version of Server.exe will always be on the repo.

```POWERSHELL
gcc Server.c -o Server.exe -lws2_32
```

and to compile the client on linux just 

```BASH
gcc linux_client.c -o client
```

and to run 

```BASH
./client <server_IP>
```
