/*
 *  Author:      Robert Blaine Wilson
 *  Date:        7/3/2023
 *  
 *  Synopsis:    This application is an HTTP fetcher and downloader. It accepts two command-line parameters: 1) The URL to an online resource. 2) The file name
 *               to save the output to. The program first splits the URL parameter into the host name and path of the resource. Then the host name is resolved
 *               an IPv4 address, and a socket is created that attempts to connect to the web server with this address on port 80. The program then builds an
 *               HTTP request including the path to the resource and sends it to the webserver. After the request has been sent, it reads the HTTP response from
 *               the server. If the status code 'HTTP/1.1 200 OK' is recieved, the program will store the body of the response into the file name specified in the
 *               second command line parameter.
 *  
 *  Help:        While writting this file, I followed along with the material provided in module 4.
 * 
 *  Compilation: g++ -c http_downloader.cpp
 *               g++ -o hdr http_downloader.o
 * 
 *  Usage:       ./hdr <URL> <Output File>
*/

#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

using namespace std;


struct URL
{
    string prefix;
    string hostname;
    string path;
};


/* Function Prototypes */
string getIPv4(string);
bool extractURL(string, URL&);



int main(int argc, char* argv[])
{
    // Validate Command Line Arguments
    if(argc != 3)
    {
        cout << "Usage: " << argv[0] << " <URL> <Output File>" << endl;
        return -1;
    }


    // Extract URL Data
    struct URL url;
    if( !extractURL((string)argv[1], url ) )
    {
        cout << "Invalid URL: " << argv[1] << endl;
        return -1;
    }
    

    // Create HTTP Client Socket
    int httpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(httpSocket < 0)
    {
        perror("HTTP Socket");
        return -1;
    }


    // Struct for Server Connection Info
    struct sockaddr_in sin_;
    sin_.sin_family = AF_INET;
    sin_.sin_port = htons(80);


    // Resolve Hostname to IPv4
    string ipv4 = getIPv4(url.hostname);
    if(ipv4.empty())
    {
        return -1;
    }
    sin_.sin_addr.s_addr = inet_addr(ipv4.c_str());


    // Connect to HTTP Server
    int result = connect(httpSocket, (const sockaddr*)&sin_, sizeof(sin_));
    if(result < 0)
    {
        perror("Connect");
        return -1;
    }


    // Construct HTTP GET Request
    string request;
    request += "GET " + url.path + " HTTP/1.1\r\n";
    request += "Host: " + url.hostname + "\r\n";
    request += "Connection: close\r\n";
    request += "Accept: text/html,text/plain\r\n\r\n";


    // Send HTTP Get Request
    ssize_t bytes;
    bytes = write(httpSocket, request.c_str(), request.length());
    if(bytes < 0)
    {
        perror("HTTP Get Request");
        close(httpSocket);
        return -1;
    }
    else if(bytes == 0)
    {
        cout << "Server Closed Connection." << endl;
        close(httpSocket);
        return 0;
    }
    

    // Recieve HTTP Response
    cout << "Downloading " << url.prefix + url.hostname + url.path << " to " << argv[2] << "...";
    char buffer[4096];
    bytes = read(httpSocket, &buffer, sizeof(buffer));
    if(bytes < 0)
    {
        perror("HTTP Response");
        close(httpSocket);
        return -1;
    }
    else if(bytes == 0)
    {
        cout << "Server Closed Connection." << endl;
        close(httpSocket);
    }
    buffer[bytes] = '\0';


    // Extract HTTP Header and Body
    string bufferString = (string)buffer;
    int i = bufferString.find("\r\n\r\n");
    if(i == string::npos)
    {
        cout << "Could not extract Header data from HTTP Response..." << endl;
        close(httpSocket);
        return -1;
    }
    string header = bufferString.substr(0, i);
    string body = bufferString.substr(i+4); // skip /r/n/r/n
    

    // Check For Successful Request
    if(header.find("HTTP/1.1 200 OK") != string::npos)
    {
        ofstream myFile;
        myFile.open(argv[2]);
        if(!myFile)
        {
            perror(argv[2]);
            return -1;
        }
        myFile << body;
        myFile.close();
        cout << "SUCCESS." << endl;
    }
    else
    {
        cout << "FAILED." << endl;
    }


    close(httpSocket);
    return 0;
}



/*
 * Function: extractURL
 * Parameters: a string representing the URL to extract, and a reference to a URL structure to store the results
 * Return: a boolean value that represents if the URL data has been successfully extracted and saved in the URL structure
 * This function extracts the prefix, host name, and path from a URL string and saves it in a URL structure. If successful, the function will return true.
*/
bool extractURL(string argv, URL &url)
{
    // First Check For Prefix
    if(argv.substr(0, 7) == "http://")
    {
        url.prefix = "http://";
    }
    else if(argv.substr(0, 8) == "https://")
    {
        url.prefix = "https://";
    }
    else
    {
        url.prefix = "";
    }

    // Prepare Hostname
    url.hostname = argv.substr(url.prefix.size());
    int pathIndex = url.hostname.find('/');
    if(pathIndex == string::npos)
    {
        // Invalid URL - No Path
        return false;
    }
    
    url.path = url.hostname.substr(pathIndex);
    url.hostname = url.hostname.substr(0, pathIndex);

    return true;
} 



/*
 * Function: getIPv4
 * Parameters: a string representing a host name to resolve
 * Return: a string representing an IPv4 address
 * This function resolves an IPv4 address from a host name parameter. If an IP address can not be resolved, an empty string will be returned.
 * (This function intentially only returns the first entry found in the linked list of addrinfo structures)
*/
string getIPv4(string hostname)
{
    struct addrinfo* ai_res;
//    struct addrinfo ai_hints;
//    ai_hints.ai_family = AF_INET;               // hint for IPv4
//    ai_hints.ai_socktype = SOCK_STREAM;         // hint for TCP


    int res = getaddrinfo(hostname.c_str(), "80", NULL, &ai_res);
    if(res)
    {
        cout << "DNS Resolution Issue" << endl;
        return "";
    }


    char ipv4[NI_MAXHOST];
    int ni_res = getnameinfo(ai_res->ai_addr, ai_res->ai_addrlen, ipv4, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if(ni_res)
    {
        cout << "Name resolution issue" << endl;
        return "";
    }

    return ipv4;
}