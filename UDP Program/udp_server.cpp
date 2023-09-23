/*
 *  Author:      Robert Blaine Wilson
 *  Date:        7/17/2023
 *  
 *  Synopsis:    This application is the UDP server for the User Datagram Protocol Program. The application accepts a socket file as a parameter
 *               and binds the server socket according to the socket file. The server ensures graceful termination by registering a signal handler
 *               and exit function. The server continuously waits for incoming UDP packets and decodes the packet once read. The application reads
 *               a maximum number of 1500 bytes. The server prints the source port, destination port, length, and checksum recieved from the UDP packet.
 *               The server also verifies the checksum to ensure the data is not corrupt. Finally, the server prints the data in hexadecimal in 8 octets 
 *               per line.
 *  
 *  Help:        While writting this file, I followed along with the material provided in module 5.
 * 
 *  Compilation: g++ -c udp_server.cpp
 *               g++ -o udp_server udp_server.o
 * 
 *  Usage:       ./udp_server <socket file>
*/

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;


/* Globals */
int serverSocket;
char* socketFile;

struct UDPHeader
{
    uint16_t sourcePort;
    uint16_t destinationPort;
    uint16_t length;
    uint16_t checksum;
};


/* Function Prototypes */
void cleanup();
void signalHandler(int);
uint16_t calculateChecksum(UDPHeader&, uint8_t*);
void printData(uint8_t*, uint16_t);


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
    serverSocket = socket(AF_UNIX, SOCK_RAW, 0);
    if(serverSocket < 0)
    {
        perror("Server Socket");
        return -1;
    }


    // describe the server socket
    sockaddr_un un;
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, socketFile, sizeof(un.sun_path)-1);


    // bind the server socket to the OS
    int result = bind(serverSocket, (sockaddr*)&un, sizeof(un));
    if(result < 0)
    {
        perror("Bind");
        return -1;
    }

    
    // register exit function
    atexit(cleanup);


    // register signal interrupt function
    signal(SIGINT, signalHandler);


    /* UDP Server */
    int MTU = 1500;         // Maximum Transmission Unit
    uint8_t buffer[MTU];    // buffer to read the UDP data
    UDPHeader udpHeader;    // struct to store UDP header data
    
    for(;;)
    {
        cout << "[UDP SERVER]: Waiting For Connection..." << endl;
        
        // read the UDP packet on the server socket
        ssize_t bytes = read(serverSocket, &buffer, sizeof(buffer));
        if(bytes <= 0)
        {
            cout << "There was an error reading UDP data on the server socket..." << endl;
            return -1;
        }
        else
        {
            cout << bytes << " byte(s) of data recieved." << endl;
            cout << "Decoding UDP" << endl;
            cout << "------------" << endl;


            // copy UDP header portion to the UDP header structure
            memcpy(&udpHeader, buffer, sizeof(udpHeader));


            // calculate the size of the data
            uint16_t dataLength = ntohs(udpHeader.length) - sizeof(udpHeader);


            // copy the UDP data portion into the UDP data array
            uint8_t data[dataLength];
            memcpy(data, buffer + sizeof(udpHeader), dataLength);


            // convert UDP header to host byte order
            udpHeader.sourcePort = ntohs(udpHeader.sourcePort);
            udpHeader.destinationPort = ntohs(udpHeader.destinationPort);
            udpHeader.length = ntohs(udpHeader.length);
            udpHeader.checksum = ntohs(udpHeader.checksum);


            // print UDP packet details
            cout << "SPORT: " << udpHeader.sourcePort << endl;
            cout << "DPORT: " << udpHeader.destinationPort << endl;
            cout << "LENGTH: " << udpHeader.length << endl;
            cout << "CKSUM: 0x" << hex << udpHeader.checksum;
            cout << dec;


            // verify checksum
            uint16_t checksum = calculateChecksum(udpHeader, data);
            if(checksum == udpHeader.checksum)
            {
                cout << "...OK." << endl;
            }
            else
            {
                cout << "...CORRUPT...0x" << hex << checksum << endl;
            }
            
            cout << dec;
            cout << dataLength << " byte(s) of data follows." << endl << endl;
            printData(data, dataLength);
            cout << endl;
        }
    }

    return 0;
}



/*
 * Function: cleanup
 * Parameters: None
 * Return: None
 * Description: This function closes the server socket and unlinks the socket file
*/
void cleanup()
{
    // close server socket
    close(serverSocket);

    // unlink socketFile
    unlink(socketFile);
}



/*
 *  Function: signalHandler
 *  Parameters: integer representing an interrupt signal
 *  Return: None
 *  Description: This function handles an interrupt signal before termination. It clears the interrupt and gracefully exits the application
*/
void signalHandler(int signal)
{
    // clear signal
    (void)signal;

    // exit
    exit(EXIT_SUCCESS);
}



/* Function: calculateChecksum
 * Parameters: A reference to a UDPHeader structure, a pointer to an array of UDP data
 * Return: a unsigned 2 byte integer representing the checksum value
 * Description: This function returns the checksum value by adding together all of the bytes in the UDPHeader structure with the bytes
 *              of the data array and then performing one's compliment on their sum.
*/
uint16_t calculateChecksum(UDPHeader& udpHeader, uint8_t* data)
{
    // add the header bytes
    uint16_t sum = udpHeader.sourcePort +
                   udpHeader.destinationPort +
                   udpHeader.length;

    // add the data bytes
    for(uint16_t i = 0; i < udpHeader.length - sizeof(udpHeader); i++)
    {
        sum += data[i];
    }

    // return one's compliment    
    return (~sum);
}



/* Function: printData
 * Parameters: a pointer to a UDP data array, a unsigned 2 byte integer representing the length of the data array
 * Return: None
 * Description: This function prints the bytes of the UDP data array as octets in hexadecimal. It prints up to 8 octets per line.
*/
void printData(uint8_t* data, uint16_t dataLength)
{
    for(uint16_t i = 0; i < dataLength; i++)
    {
        // 8 octets per line
        if(((i % 8) == 0) && i != 0)
        {
            cout << endl;
        }

        // add '0' for formatting
        if(data[i] <= 15)
        {
            cout << "0";
        }

        // output the data octet in hexadecimal
        cout << hex << static_cast<uint16_t>(data[i]) << " ";
    }

    // reset the output stream to decimal
    cout << dec;
    cout << endl;
}