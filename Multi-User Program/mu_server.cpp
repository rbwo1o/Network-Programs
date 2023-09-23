/*
 *  Author:      Robert Blaine Wilson
 *  Date:        6/25/2023
 *  
 *  Synopsis:    This file is the server for the Multi-User program. It uses the AF_UNIX address family and takes one command line argument which is
 *               the socket file to create. It takes an asynchronous approach to handling multiple clients by storing client sockets in a vector 
 *               and making the server socket non-blocking when there are clients connected. This file takes advantage of the select() function 
 *               to see whenever data is waiting to be read on socket file descriptors. After a handshake with each client, the server reads commands
 *               sent from the client until the command 'quit' has been sent. After this, the server closes the client socket and removes the socket 
 *               from the client vector data structure. If there are no clients, the server socket is set to block.
 *  
 *  Help:        While writting this file, I followed along with the material provided in module 3. I also asked a question in piazza regarding how to handle
 *               abrupt client socket disconnects.
 * 
 *  Compilation: g++ -c mu_server.cpp
 *               g++ -o mu_server mu_server.o
 * 
 *  Usage:       ./mu_server <socket file>
*/

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <sys/signal.h>

using namespace std;


/* Globals */
int serverSocket;
char* socketFile;
struct clientSocketStruct
{
    int id;
    int socket;
    struct sockaddr_un un;
    socklen_t size;
};
vector<clientSocketStruct*> clientSockets;


/* Function Prototypes */
void cleanup();
void signalHandler(int);
int prepareFDReadSet(fd_set&);
void closeSocket(clientSocketStruct*);



int main(int argc, char* argv[])
{
    // validate command line arguments
    if(argc != 2)
    {
        cout << "Usage: " << argv[0] << " <socket file>" << endl;
        return -1;
    }
    socketFile = argv[1];


    // create server socket
    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(serverSocket < 0)
    {
        perror("server socket");
        return -1;
    }


    // decorate server socket
    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, socketFile, sizeof(un.sun_path)-1);


    // bind server socket to OS
    int result = bind(serverSocket, (struct sockaddr*)&un, sizeof(un));
    if(result < 0)
    {
        perror("bind");
        return -1;
    } 


    // listen for connections on server socket
    result = listen(serverSocket, 10);
    if(result < 0)
    {
        perror("listen");
        return -1;
    }


    // register exit function
    atexit(cleanup);


    // register interrupt handler function
    signal(SIGINT, signalHandler);
    


    /* Asynchronous Client Socket Handling*/

    fd_set readset;             // read set for file descriptors
    
    struct timeval timeout;     // interval that select() block waiting for data on a file decriptor
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    struct timespec ts;         // interval used for nanosleep()

    int count = 0;              // history of the number of clients handled by the application
    int maxFD;                  // the largest file descriptor size

    char buffer[100];           // read buffer
    ssize_t bytes;

    for(;;)
    {
        if(clientSockets.size() == 0)
        {
            cout << "No clients, blocking on server socket..." << endl;
            // set server socket to block
            fcntl(serverSocket, F_SETFL, fcntl(serverSocket, F_GETFL) & ~O_NONBLOCK);

            // prepare for first client socket
            struct clientSocketStruct* clientSocket = new clientSocketStruct;
            clientSocket->socket = accept(serverSocket, (struct sockaddr*)&clientSocket->un, &clientSocket->size);
            clientSocket->id = ++count;

            // inform client of connection (handshake protocol)
            write(clientSocket->socket, "HELLO", sizeof("HELLO"));
            
            // save client socket
            clientSockets.push_back(clientSocket);
        }
        else
        {
            // change server socket to non-blocking
            fcntl(serverSocket, F_SETFL, fcntl(serverSocket, F_GETFL) | O_NONBLOCK);

            // prepare file descriptor read set
            maxFD = prepareFDReadSet(readset);

            // check data on file descriptors in prepared read set
            if(0 == (select(maxFD+1, &readset, NULL, NULL, &timeout)))
            {
                ts.tv_sec = 0;
                ts.tv_nsec = 1000000;
                nanosleep(&ts, NULL);
            }
            else
            {
                // check server socket for new connection
                if(FD_ISSET(serverSocket, &readset))
                {
                    // prepare for new client socket
                    struct clientSocketStruct* clientSocket = new clientSocketStruct;
                    clientSocket->socket = accept(serverSocket, (struct sockaddr*)&clientSocket->un, &clientSocket->size);
                    clientSocket->id = ++count;

                    // inform client of connection (handshake protocol)
                    write(clientSocket->socket, "HELLO", sizeof("HELLO"));

                    // save client socket
                    clientSockets.push_back(clientSocket);
                }
                // check saved client sockets for data
                for(int i=0; i < clientSockets.size(); i++)
                {
                    if(FD_ISSET(clientSockets.at(i)->socket, &readset))
                    {
                        bytes = read(clientSockets.at(i)->socket, buffer, sizeof(buffer));
                        if(bytes < 0)
                        {
                            perror("client socket " + clientSockets.at(i)->id);

                            // error reading -> close socket
                            closeSocket(clientSockets.at(i));

                            // remove saved client from vector
                            clientSockets.erase(clientSockets.begin() + i);
                        }
                        else if(bytes == 0)
                        {
                            cout << "client " << clientSockets.at(i)->id << " has closed the connection." << endl;

                            // client closed -> close socket
                            closeSocket(clientSockets.at(i));

                            // remove saved client from vector
                            clientSockets.erase(clientSockets.begin() + i);
                        }
                        else
                        {
                            buffer[bytes] = '\0';
                            cout << "Client " << clientSockets.at(i)->id << " says '" << buffer << "'" << endl;
                            if(!strcmp(buffer, "quit\0"))
                            {
                                cout << "Client " << clientSockets.at(i)->id << " quit, see ya." << endl;

                                // client quit -> close socket
                                closeSocket(clientSockets.at(i));

                                // remove saved client from vector
                                clientSockets.erase(clientSockets.begin() + i);
                            }
                            else
                            {
                                write(clientSockets.at(i)->socket, "ENTERCMD", sizeof("ENTERCMD"));
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}



/*
 *  Function: cleanup
 *  Parameters: None
 *  Return: None
 *  Description: This function cleans up the application before termination. It closes sockets and unlinks the socket file.
*/
void cleanup()
{
    // close server socket
    close(serverSocket);

    // cleanup saved client sockets
    for(int i=0; i < clientSockets.size(); i++)
    {
        closeSocket(clientSockets.at(i));
    }

    // unlink socket file
    unlink(socketFile);
}



/*
 *  Function: signalHandler
 *  Parameters: integer representing an interrupt signal
 *  Return: None;
 *  Description: This function handles an interrupt signal before termination. It clears the interrupt and gracefully exits the application
*/
void signalHandler(int signal)
{
    // clear signal
    (void)signal;

    // exit
    exit(EXIT_SUCCESS);
}



/*
 *  Function: prepareFDReadSet
 *  Parameters: a reference to a file descriptor set
 *  Return: The largest file descriptor to be used by the select() function;
 *  Description: This function prepares the filde descriptor readset to be used by the select function. It will add the server socket file descriptor and saved 
 *               client socket file descriptors to the readset.
*/
int prepareFDReadSet(fd_set &readset)
{
    int maxFD = 0;

    // clear readset
    FD_ZERO(&readset);
    
    // add serverSocket
    FD_SET(serverSocket, &readset);
    
    // add saved client sockets
    for(int i=0; i < clientSockets.size(); i++)
    {
        //cout << "Adding " << clientSockets.at(i)->id << " ";
        FD_SET(clientSockets.at(i)->socket, &readset);
        if(clientSockets.at(i)->socket > maxFD)
        {
            maxFD = clientSockets.at(i)->socket;
        }
    }

    return maxFD;
}



/*
 *  Function: closeSocket
 *  Parameters: pointer to a dynamically allocated clientSocketStruct structure
 *  Return: None
 *  Description: This function closes the client socket and frees the dymanically allocated memory of the clientSocketStruct structure
*/
void closeSocket(clientSocketStruct* clientSocket)
{
    // close the client socket
    close(clientSocket->socket);

    // free allocated memory
    delete clientSocket;
}