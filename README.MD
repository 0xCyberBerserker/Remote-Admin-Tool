This software is intended to be a Remote Administration Tool developed in C for a Windows Agent and a Linux Server.
It is intended to have some personalized commands and execute commands on powershell by the argument personalized command exec and then the powershell command you want to execute on the agent.

Usable commands:

    cd directory             

    pwd                         - Shows the full path of the directory we are in

    ls                          - Just like a Dir or an ls on a Linux Machine

    put local_file              - Upload a file to the server

    get remote_file             - Download a file from the server

    exec Powershell_Command     - Execute a powershell command on the server

    exit                        - Exits the client


**Compilation:**

You need to compile the agent on a Windows machine and need MinGW and GCC installed.

Latest compiled version of agent1.exe will always be on the repo.

```POWERSHELL
gcc agent1.c -o agent1.exe -lws2_32
```

and to compile the client on linux just 

```BASH
gcc linux_server.c -o server
```

and to run 

```BASH
./server <agent_IP>
```

Working Tests:

![https://github.com/JohnMorgan1234/Remote-Admin-Tool/blob/main/bichito/GIFs/agent2_test.gif](https://github.com/JohnMorgan1234/Remote-Admin-Tool/blob/main/bichito/GIFs/agent2_test.gif)

![https://github.com/JohnMorgan1234/Remote-Admin-Tool/blob/main/bichito/GIFs/Bishito2.gif](https://github.com/JohnMorgan1234/Remote-Admin-Tool/blob/main/bichito/GIFs/Bishito2.gif)




