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

static const string HTTP_OK = "HTTP/1.1 200 OK\r\n"                  \
                              "Content-Type: application/json\r\n"   \
                              "Content-Length: 21\r\n"               \
                              "{\"result\":\"allowed\"}";
static const string HTTP_SERVICE_UNAVAILABLE = "HTTP/1.1 503 Service Unavailable\r\n" \
                              "Content-Type: application/json\r\n"                    \
                              "Content-Length: 25\r\n"                                \
                              "{\"result\":\"not allowed\"}";

struct IncomingRequest
{
    int client_id;
    int client_port;
};

static bool allow_request()
{
    return false;
}

static bool send_response(int socket, int client_id)
{
    const string &response = client_id == 0 ? HTTP_OK : HTTP_SERVICE_UNAVAILABLE;
    int bytes_sent = send(socket, response.c_str(), response.length(), MSG_NOSIGNAL);
    return bytes_sent == response.length();
}

static int extract_client_id(const string &request)
{
    const string client_id_str = "?clientId=";
    auto client_id_idx = request.find_first_of(client_id_str);
    auto first_line_end_idx = request.find_first_of("\r\n");
    if (request.rfind("GET", 0) == 0 &&
        client_id_idx != string::npos &&
        first_line_end_idx != string::npos &&
        client_id_idx < first_line_end_idx)
    {
        auto client_id_pos = client_id_idx + client_id_str.size();
        auto client_id_str = request.substr(client_id_pos,
                                            first_line_end_idx - client_id_pos);
        return stoi(client_id_str);
    }

    return -1;
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

    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        cout << "Unable to set socket option" << endl;
        close(sockfd);
        return;
    }

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

    char buffer[256];
    int bytes_read = read(newsockfd, buffer, 255);
    if (bytes_read < 0)
    {
        cout << "Unable to read from netsockfd" << endl;
        close(newsockfd);
        close(sockfd);
        return;
    }
    else
    {
        cout << "Got from client: " << buffer << endl;
    }

    string request(buffer, bytes_read);

    int client_id_from_request = extract_client_id(request);
    if (client_id_from_request < 0)
    {
        cout << "Illegal client request: " << request << endl;
        close(newsockfd);
        close(sockfd);
        return;
    }

    if (!send_response(newsockfd, client_id_from_request))
    {
        cout << "Unable to send response to client" << endl;
        close(newsockfd);
        close(sockfd);
        return;
    }

    cout << "Done handling request" << endl;

    close(newsockfd);
    close(sockfd);
}

static void do_serve()
{
    while (!exit_thread_flag)
    {
        http_listen();
        sleep(1);
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
