// std
#include <atomic>
#include <map>
#include <array>
#include <iostream>
#include <cstring>

// platform-specific
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

// local
#include "dos_server.h"

using namespace std;

static const int SERVER_PORT = 8080;

static atomic<bool> exit_thread_flag{false};

// client_id -> last incoming requests for that client
static map<int, array<long, 5>> last_requests;

struct IncomingRequest
{
    int client_id;
    int client_port;
};

static bool allow_request()
{
    return false;
}

static void http_listen()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cout << "Unable to create socket" << endl;
        return;
    }

    cout << "Socket " << sockfd << " created" << endl;

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    //inet_pton(AF_INET, INADDR_ANY, &(serv_addr.sin_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Unable to bind address" << endl;
        close(sockfd);
        return;
    }

    listen(sockfd, 5);

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (newsockfd < 0)
    {
        cout << "Unable to accept new connection" << endl;
        close(sockfd);
        return;
    }

    cout << "Got new connection from " << inet_ntoa(client_addr.sin_addr)
         << ":" << ntohs(client_addr.sin_port) << endl;

    close(newsockfd);
    close(sockfd);
}

static void do_serve()
{
    while (!exit_thread_flag)
    {
        http_listen();
    }
}

DosServer::DosServer() :
    m_runner(do_serve)
{
}

void DosServer::stop()
{
    cout << "Stopping server" << endl;
    exit_thread_flag = true;
    m_runner.join();
    cout << "Server " << " now stopped" << endl;
}
