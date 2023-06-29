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

static void server_runner(SafeMsgQueue<DosServer::RequestMsg> &msg_queue)
{
    while (!exit_thread_flag)
    {
        auto msg = msg_queue.dequeue();

        int client_id_from_request = extract_client_id(msg.request);
        if (client_id_from_request < 0)
        {
            if (client_id_from_request == -1)
            {
                cout << "Stop requested. Shutting down..." << endl;
            }
            else
            {
                cout << "Illegal client request: " << msg.request << endl;
                close(msg.sockfd);
            }
            continue;
        }

        if (!send_response(msg.sockfd, client_id_from_request))
        {
            cout << "Unable to send response to client" << endl;
            close(msg.sockfd);
            continue;
        }

        close(msg.sockfd);
    }
}

static void server_thread(int sockfd, SafeMsgQueue<DosServer::RequestMsg> &msg_queue)
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    while (!exit_thread_flag)
    {
        int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (newsockfd < 0)
        {
            cout << "Unable to accept new connection" << endl;
            continue;
        }

        cout << "Got new connection from " << inet_ntoa(client_addr.sin_addr)
             << ":" << ntohs(client_addr.sin_port) << endl;

        char buffer[256];
        int bytes_read = read(newsockfd, buffer, 255);
        if (bytes_read < 0)
        {
            cout << "Unable to read from netsockfd" << endl;
            close(newsockfd);
            continue;
        }
        else
        {
            cout << "Got from client: " << buffer << endl;
        }

        string request(buffer, bytes_read);
        DosServer::RequestMsg msg = { request, newsockfd };
        msg_queue.enqueue(msg);
    }
}

static int http_listen()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cout << "Unable to create socket" << endl;
        return -1;
    }

    cout << "Socket " << sockfd << " created" << endl;

    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        cout << "Unable to set socket option" << endl;
        close(sockfd);
        return -1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Unable to bind address" << endl;
        close(sockfd);
        return -1;
    }

    listen(sockfd, 5);

    return sockfd;
}

static void do_serve(SafeMsgQueue<DosServer::RequestMsg> &msg_queue)
{
    while (!exit_thread_flag)
    {
        http_listen();
    }
}

DosServer::DosServer() :
    m_sockfd(http_listen()),
    m_server(thread([&]{server_thread(m_sockfd, m_msg_queue);}))
{
    auto processor_count = thread::hardware_concurrency();
    if (processor_count == 0)
    {
        processor_count = 2;
    }

    cout << "Creating " << processor_count << " runners..." << endl;
    for (auto i = 0; i < processor_count; i++)
    {
        m_runners.push_back(thread([&]{server_runner(m_msg_queue);}));
    }
}

DosServer::~DosServer()
{
    stop();
}

void DosServer::stop()
{
    if (!exit_thread_flag)
    {
        cout << "Stopping server" << endl;

        exit_thread_flag = true;

        const DosServer::RequestMsg stop_msg = { string{}, -1 };
        for (int i = 0; i < m_runners.size(); i++) m_msg_queue.enqueue(stop_msg);
        cout << "Awaiting runners to shut down..." << endl;
        for (auto &t : m_runners) t.join();

        close(m_sockfd);

        cout << "Awaiting server to shut down..." << endl;
        m_server.join();

        cout << "Server " << " now stopped" << endl;
    }
}
