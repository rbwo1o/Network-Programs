/*
 *  Author:      Robert Blaine Wilson
 *  Date:        7/17/2023
 *  
 *  Synopsis:    This application is the UDP client for the User Datagram Protocol Program. The application accepts two parameters: 1) A socket file to connect to
 *               2) An integer representing the value to seed a random number. After connecting to the server, the client seeds the random number generator with
 *               the second command line parameter provided by the user. If no seed is provided, it seeds the random number generator with time(NULL). It then builds
 *               a UDP packet according to the following fields:
 *              
 *               Data: The size is a random number between 50 and 100. Each byte is a random number between 0 and 255.
 *               Source Port: A random number between 0 and 65,535
 *               Destination Port: A random number between 0 and 65,535
 *               Length: The size of the entire UDP packet
 *               Cheksum: A unsigned short calculated by the calculateChecksum function.
 * 
 *               The maxmum bytes that the server will send is 1500 bytes. The client will print out all of the UDP data so that it can easily be compared to the data
 *               output by the server.
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
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;


struct UDPHeader
{
    uint16_t sourcePort;
    uint16_t destinationPort;
    uint16_t length;
    uint16_t checksum;
};


/* Function Prototypes */
uint16_t calculateChecksum(UDPHeader&, uint8_t*);
void printData(uint8_t*, uint16_t);


int main(int argc, char* argv[])
{
    // validate command line arguments
    if(argc != 2 && argc != 3)
    {
        cout << "Usage: " << argv[0] << " <socket file> [seed]" << endl;
        return -1;  
    }


    // check if seed has been provided by command line arguments
    bool hasSeed = false;
    if(argc == 3)
    {
        hasSeed = true;
    }


    // create the client socket using AF_UNIX and SOCK_RAW
    int clientSocket = socket(AF_UNIX, SOCK_RAW, 0);
    if(clientSocket < 0)
    {
        perror("Client Socket");
        return -1;
    }


    // describe the client socket
    sockaddr_un un;
    un.sun_family = AF_UNIX;
    strncpy(un.sun_path, argv[1], sizeof(un.sun_path) - 1);


    // connect to the socket file
    int result = connect(clientSocket, (sockaddr*)&un, sizeof(un));
    if(result < 0)
    {
        perror("Connect");
        return -1;
    }


    // seed the random number generator
    if(hasSeed)
    {
        int seed = atoi(argv[2]);
        srand((unsigned) seed);
    }
    else
    {
        srand((unsigned) time(NULL));
    }


    // Initialize a new UDP packet header
    UDPHeader udpHeader;


    // generate the data size which is a number between 50 and 100
    int num = 50 + (rand() % 51);
    uint16_t dataLength = static_cast<uint16_t>(num);
    udpHeader.length = dataLength + sizeof(udpHeader);


    // Initialize the data array
    uint8_t data[dataLength];
    // generate random bytes for the data in the packet
    for(int i = 0; i < num; i++)
    {
        // byte is a number between 0 and 255
        int rbytes = rand() % 256;
        uint8_t b = static_cast<uint8_t>(rbytes);
        data[i] = b;
    }


    // generate source port which is a number between 0 and 65535
    int rSourcePort = rand() % 65536;
    uint16_t sourcePort = static_cast<uint16_t>(rSourcePort);
    udpHeader.sourcePort = sourcePort;
    

    // generate destination port which is a number between 0 and 65535
    int rDestinationPort = rand() % 65536;
    uint16_t destinationPort = static_cast<uint16_t>(rDestinationPort);
    udpHeader.destinationPort = destinationPort;
    

    // calculate checksum
    udpHeader.checksum = calculateChecksum(udpHeader, data);


    // print UDP header
    cout << "SPORT: " << udpHeader.sourcePort << endl;
    cout << "DPORT: " << udpHeader.destinationPort << endl;
    cout << "LENGTH: " << udpHeader.length << " (data is " << dataLength << " bytes)" << endl;
    cout << "CKSUM: 0x" << hex << udpHeader.checksum << endl;
    cout << "DATA" << endl;
    cout << "~~~~" << endl;
    printData(data, dataLength);


    // convert the UDP header to network byte order
    udpHeader.sourcePort = htons(udpHeader.sourcePort);
    udpHeader.destinationPort = htons(udpHeader.destinationPort);
    udpHeader.length = htons(udpHeader.length);
    udpHeader.checksum = htons(udpHeader.checksum);


    // copy the UDP header and data into a single stream to be sent to the server
    uint8_t buffer[sizeof(UDPHeader) + dataLength];
    memcpy(buffer, &udpHeader, sizeof(udpHeader));
    memcpy(buffer + sizeof(udpHeader), data, sizeof(data));



    int MTU = 1500;
    // ensure the size of the UDP packet is less than or equal to the MTU
    if(sizeof(buffer) <= MTU)
    {
        // write the UDP packet to the server
        ssize_t bytes = write(clientSocket, buffer, sizeof(buffer));
        if(bytes < 0)
        {
            cout << "There was an error sending the UDP packet to the server..." << endl;
            close(clientSocket);
            return -1;
        }
        else if(bytes == 0)
        {
            cout << "The connection was closed by the server..." << endl;
            close(clientSocket);
            return 0;
        }
    }
    else
    {
        cout << "The size of the UDP packet is greater that the Maximum Transmission Unit of 1,500 bytes..." << endl;
        close(clientSocket);
        return -1;
    }



    // close the socket
    close(clientSocket);
    return 0;
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
}