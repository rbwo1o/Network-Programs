/*
* Author: Robert Blaine Wilson
* Date: 6/19/2023
* Synopsis: This file is the server for the Peer-to-Peer program. It used the AF_UNIX address family and accepts one
*           command line argument, which is the socket file to create. Then this file initializes a socket and listens
*           for incoming connections. After a handshake with the client, the socket reads commands sent from the client 
*           until the command 'quit' has been sent. After this, the server closes the socket, unlinks the socket file,
*           and ends the program.
* Help: While writting this file, I followed along the material provided in Module 2.
* Compilation: g++ -c p2p_server.cpp
*              g++ -o p2p_server p2p_server.o
*/

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
    // Validate the socket file command line argument to ensure the OS can later bind the socket.
    if(argc != 2)
    {
        std::cout << "Expecting a single command line argument, which is the socket file to create." << std::endl;
        std::cout << "i.e: ./p2p_server socketFile.sock" << std::endl;
        return -1;
    }


    // Initialize a new socket to be used by the server, if the return value is negative then there are errors.
    int serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(serverSock < 0)
    {
        std::cout << "Could not initialize server socket..." << std::endl;
        return -1;
    }


    // Initialize the un sockaddress structure. This decorates the AF_UNIX socket to be later bound by the OS.
    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, argv[1], sizeof(un.sun_path) - 1);


    // Bind the socket to the OS. A negative return value indicates an error.
    int result = bind(serverSock, (const sockaddr*)&un, sizeof(un));
    if(result < 0)
    {
        std::cout << "Error binding the socket to the Operating System..." << std::endl;
        close(serverSock);
        return -1;
    }


    // Listen for incoming connections on the bound socket. A negative return value indicates an error.
    result = listen(serverSock, 5);
    if(result < 0)
    {
        std::cout << "Error listening on the socket for incoming connections..." << std::endl;
        close(serverSock);
        unlink(argv[1]);
        return -1;
    }


    // Accept an incoming connection on the server socket. When a client has connected, a new dedicated socket is used for the connection
    struct sockaddr_un clientAddr;
    socklen_t csize;
    int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &csize);



    /* HANDSHAKE PROTOCOL */
    char writeBuffer[100];      // write buffer to be used
    char readBuffer[100];       // read buffer to be used
    ssize_t bytes;

    // send initial response to the client show they have successfully connected. 
    // -- 0 bytes returned indicates the client has closed the connection.
    // -- negative bytes returned indicates there was an error sending data to the client
    strcpy(writeBuffer, "HELLO");
    bytes = write(clientSock, writeBuffer, sizeof(writeBuffer));
    if(bytes == 0)
    {
        std::cout << "The socket has been closed by the client..." << std::endl;
        close(serverSock);
        close(clientSock);
        unlink(argv[1]);
        return 0;
    }
    else if(bytes < 0)
    {
        std::cout << "There was an error writting bytes to the socket..." << std::endl;
        close(clientSock);
        close(serverSock);
        unlink(argv[1]);
        return -1;
    }
    

    // read handshake response from the client
    // -- 0 bytes returned indicates the client has closed the connection.
    // -- negative bytes returned indicates there was an error reading from the client.
    bytes = read(clientSock, readBuffer, sizeof(readBuffer));
    if(bytes == 0)
    {
        std::cout << "The socket has been closed by the client..." << std::endl;
        close(clientSock);
        close(serverSock);
        unlink(argv[1]);
        return 0;
    }
    else if(bytes < 0)
    {
        std::cout << "There was an error reading bytes from the socket..." << std::endl;
        close(clientSock);
        close(serverSock);
        unlink(argv[1]);
        return -1;
    }
    else
    {
        readBuffer[bytes] = '\0';
        std::cout << "Client says '";
        std::cout << readBuffer;
        std::cout << "'" << std::endl;
    }


    // handshake protocol is now validated. Loop to accept commands from client can now be started.
    strcpy(writeBuffer, "ENTERCMD");
    while(true){
        // tell the client to enter a command
        bytes = write(clientSock, writeBuffer, sizeof(writeBuffer));
        if(bytes == 0)
        {
            std::cout << "The socket has been closed by the client..." << std::endl;
            break;
        }
        else if(bytes < 0)
        {
            std::cout << "There was an error writting to the socket..." << std::endl;
            break;
        }

        // read command from the client
        bytes = read(clientSock, readBuffer, sizeof(readBuffer));
        if(bytes == 0)
        {
            std::cout << "The socket was closed by the client..." << std::endl;
            break;
        }
        else if(bytes < 0)
        {
            std::cout << "There was an error reading from the socket..." << std::endl;
            break;
        }
        else
        {
            // If the command 'quit' has been recieved, then exit the server.
            if(strcmp(readBuffer, "quit") == 0)
            {
                std::cout << "Client quit, see ya" << std::endl;
                break;
            }
            else
            {
                readBuffer[bytes] = '\0';
                std::cout << "Client says '";
                std::cout << readBuffer;
                std::cout << "'" << std::endl;
            }
        }
    }

    // close the server socket
    close(serverSock);
    // close the client socket
    close(clientSock);
    // unlink the bound socket file
    unlink(argv[1]);

    return 0;
}