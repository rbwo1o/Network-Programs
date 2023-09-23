/*
* Author: Robert Blaine Wilson
* Date: 6/19/2023
* Synopsis: This file is the client for the Peer-to-Peer program. It uses the AF_UNIX address family and accepts one
*           command line argument, which is the socket file to use. The client initializes a socket and connects 
*           to the server listening on the socket file. After a handshake with the server, the socket sends commands
*           to the server until the command 'quit' is entered. After the client sends the 'quit' command, the client 
*           closes the socket and ends the program.
* Help: While writting this file, I followed along the material provided in Module 2.
* Compilation: g++ -c p2p_client.cpp
*              g++ -o p2p_client p2p_client.o
*/

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
    // Validate the socket file command line argument to ensure the client will have a file to attempt to connect to.
    if(argc != 2)
    {
        std::cout << "Expecting a single command line argument, which is the socket file to use." << std::endl;
        std::cout << "i.e: ./p2p_client socketFile.sock" << std::endl;
        return -1;
    }


    // Initialize a new socket to be used by the client, if the return value is negative then there are errors.
    int clientSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(clientSock < 0)
    {
        std::cout << "Could not initialize socket..." << std::endl;
        return -1;
    }


    // Initialize the un sockaddress structure. This decorates the AF_UNIX socket to be later bound by the OS. 
    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, argv[1], sizeof(un.sun_path) - 1);


    // Attempt to connect to the server, if the return value is negative then there are errors.
    int result = connect(clientSock, (const struct sockaddr*)&un, sizeof(un));
    if(result < 0)
    {
        std::cout << "Error connecting the socket..." << std::endl;
        perror("connect");
        close(clientSock);
        return -1;
    }


    /* HANDSHAKE PROTOCOL */
    char writeBuffer[100];      // write buffer to be used
    char readBuffer[100];       // read buffer to be used
    ssize_t bytes;

    // read initial response from the server, and see if the connection was successful
    // -- 0 bytes returned indicates the server has closed the connection.
    // -- negative bytes returned indicates there was an error reading data from the server
    bytes = read(clientSock, readBuffer, sizeof(readBuffer));
    if(bytes == 0)
    {
        std::cout << "The socket has been closed by the server..." << std::endl;
        close(clientSock);
        return 0;
    }
    else if(bytes < 0)
    {
        std::cout << "There was en error reading from the socket..." << std::endl;
        close(clientSock);
        return -1;
    }
    else
    {
        readBuffer[bytes] = '\0';
        std::cout << "Server says '";
        std::cout << readBuffer;
        std::cout << "'" << std::endl;
    }


    // write handshake response to the server.
    // -- 0 bytes returned indicates the server has closed the connection.
    // -- negative bytes returned indicates there was an error sending data to the server
    strcpy(writeBuffer, "THANKS");
    bytes = write(clientSock, writeBuffer, sizeof(writeBuffer));
    if(bytes == 0)
    {
        std::cout << "The socket has been closed by the server..." << std::endl;
        close(clientSock);
        return 0;
    }
    else if(bytes < 0)
    {
        std::cout << "There was an error writting to the socket..." << std::endl;
        close(clientSock);
        return -1;
    }
    

    // handshake protocol is now validated. Loop to send commands from server can now be started.
    while(true)
    {
        // read command text from the server
        bytes = read(clientSock, readBuffer, sizeof(readBuffer));
        if(bytes == 0)
        {
            std::cout << "The socket was closed by the server..." << std::endl;
            break;
        }
        else if(bytes < 0)
        {
            std::cout << "There was an error reading from the socket..." << std::endl;
            break;
        }
        else
        {
            readBuffer[bytes] = '\0';
            std::cout << readBuffer << ": ";
        }

        // get command text from console
        std::cin.getline(writeBuffer, sizeof(writeBuffer));

        // write command to the server
        bytes = write(clientSock, writeBuffer, sizeof(writeBuffer));
        if(bytes == 0)
        {
            std::cout << "The socket was closed by the server..." << std::endl;
            break;
        }
        else if(bytes < 0)
        {
            std::cout << "There was an error writting to the socket..." << std::endl;
            break;
        }
        else
        {
            // If the command 'quit' has been sent, then exit the client.
            if(strcmp(writeBuffer, "quit") == 0)
            {
                std::cout << "Quitting!" << std::endl;
                break;
            }
        }
    }

    // close the client socket
    close(clientSock);

    return 0;
}