#include <iostream>
#include <netdb.h>
#include <sys/socket.h>

using namespace std;

int main(int argc, char* argv[])
{
    struct addrinfo* res; 
    struct addrinfo ai_hint;
    ai_hint.ai_family = AF_INET;
    ai_hint.ai_socktype = SOCK_STREAM;

    int ar_res = getaddrinfo(argv[1], "80", &ai_hint, &res);

    if(ar_res)
    {
        cout << "Bad URL" << endl;
        return 0;
    }

    struct addrinfo* tmp;
    for(tmp = res; tmp != NULL; tmp = tmp->ai_next)
    {
        cout << "--- DNS Result ---" << endl;

        char buf[NI_MAXHOST];
        int ni_res = getnameinfo(tmp->ai_addr, tmp->ai_addrlen, buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if(ni_res)
        {
            cout << "Name error" << endl;
            return 0;
        }
        cout << buf << endl;
        cout << endl;

        break;
    }


    return 0;
}