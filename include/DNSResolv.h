#ifndef RESOLVER_H
#define RESOLVER_H
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#pragma comment(lib, "ws2_32")

static WSADATA g_wsa_data;

#endif
#ifdef linux
#include <sys/socket.h>
#include <netdb.h>
#endif

class Resolver {

public:
    Resolver() {
        initialize();
    }
    ~Resolver() {
        deinitialize();
    }
    std::vector<std::string> resolve(const char* hostname)
    {
        std::vector<std::string> out;

#ifdef _WIN32
        struct addrinfo hints, * results, * item;
        int status;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;  /* AF_INET6 to force version */
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(hostname, NULL, &hints, &results)) != 0)
        {
            return out;
        }

        for (item = results; item != NULL; item = item->ai_next)
        {
            void* addr;
            char* ipver = new char[5];
            /* get pointer to the address itself */
            /* different fields in IPv4 and IPv6 */
            if (item->ai_family == AF_INET)  /* address is IPv4 */
            {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)item->ai_addr;
                addr = &(ipv4->sin_addr);
                out.push_back(inet_ntoa(ipv4->sin_addr));
                strcpy_s(ipver, 5, "IPv4");
            }
            else  /* address is IPv6 */
            {
                continue; // no support for IPv6 yet, so ignore
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)item->ai_addr;
                addr = &(ipv6->sin6_addr);
                strcpy_s(ipver, 5, "IPv6");
            }

            /* convert IP to a string and print it */
            //inet_ntop_a(item->ai_family, addr, ipstr, sizeof ipstr);
            
        }

        freeaddrinfo(results);
        return out;

#endif
#ifdef linux
        struct addrinfo hints, * results, * item;
        int status;
        char ipstr[INET6_ADDRSTRLEN];


        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;  /* AF_INET6 to force version */
        hints.ai_socktype = SOCK_STREAM;


        if ((status = getaddrinfo(hostname, NULL, &hints, &results)) != 0)
        {
            fprintf(stderr, "failed to resolve hostname \"%s\": %s", hostname, gai_strerror(status));
            return;
        }


        printf("IP addresses for %s:\n\n", hostname);


        for (item = results; item != NULL; item = item->ai_next)
        {
            void* addr;
            char* ipver;


            /* get pointer to the address itself */
            /* different fields in IPv4 and IPv6 */
            if (item->ai_family == AF_INET)  /* address is IPv4 */
            {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)item->ai_addr;
                addr = &(ipv4->sin_addr);
                ipver = "IPv4";
            }
            else  /* address is IPv6 */
            {
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)item->ai_addr;
                addr = &(ipv6->sin6_addr);
                ipver = "IPv6";
            }


            /* convert IP to a string and print it */
            inet_ntop(item->ai_family, addr, ipstr, sizeof ipstr);
            out.push_back(ipstr);
        }


        freeaddrinfo(results);
        return out;
#endif
    }


    std::vector<std::string> resolve(std::string& hostname)
    {
        std::vector<std::string> out;

#ifdef _WIN32
        struct addrinfo hints, * results, * item;
        int status;
        char ipstr[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;  /* AF_INET6 to force version */
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(hostname.c_str(), NULL, &hints, &results)) != 0)
        {
            return out;
        }

        for (item = results; item != NULL; item = item->ai_next)
        {
            void* addr;
            char* ipver = new char[5];
            /* get pointer to the address itself */
            /* different fields in IPv4 and IPv6 */
            if (item->ai_family == AF_INET)  /* address is IPv4 */
            {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)item->ai_addr;
                addr = &(ipv4->sin_addr);
                out.push_back(inet_ntoa(ipv4->sin_addr));
                strcpy_s(ipver, 5, "IPv4");
            }
            else  /* address is IPv6 */
            {
                continue; // no support for IPv6 yet, so ignore
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)item->ai_addr;
                addr = &(ipv6->sin6_addr);
                strcpy_s(ipver, 5, "IPv6");
            }

            /* convert IP to a string and print it */
            //inet_ntop_a(item->ai_family, addr, ipstr, sizeof ipstr);

        }

        freeaddrinfo(results);
        return out;
#endif
    }
private:
    char initialize(void)
    {
#ifdef linux
        return 1;
#endif

#ifdef _WIN32
        return (WSAStartup(MAKEWORD(2, 2), &g_wsa_data) == NO_ERROR);
#endif
    }

    void deinitialize(void)
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

};

#endif