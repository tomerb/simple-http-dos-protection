// std
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <sstream>

// platform-specific
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

// local
#include "dos_client.h"

using namespace std;

static const int CLIENT_BIND_PORT_START = 9250;

static atomic<bool> exit_thread_flag{false};

static void send_http_request(int id, string &host_addr, int host_port)
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
    inet_pton(AF_INET, host_addr.c_str(), &(serv_addr.sin_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(host_port);
    //memcpy(&serv_addr.sin_addr, host_addr.c_str(), host_addr.size());
    cout << "Connecting to " << inet_ntoa(serv_addr.sin_addr) << ":" << serv_addr.sin_port << "..." << endl;
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Unable to connect to server" << endl;
        close(sockfd);
        return;
    }

    stringstream ss;
    ss << "GET /?clientId=" << id << "\r\n"
       << "Accept: application/json\r\n"
       << "\r\n\r\n";
    string request = ss.str();

    if (send(sockfd, request.c_str(), request.length(), MSG_NOSIGNAL) != (int)request.length()) {
        cout << "Error sending request." << endl;
        close(sockfd);
        return;
    }

    char cur;
    while ( read(sockfd, &cur, 1) > 0 ) {
        cout << cur;
    }

    cout << "Closing socket " << sockfd << "..." << endl;
    close(sockfd);
}

static void do_dos(int id, string host_addr, int host_port)
{
    while (!exit_thread_flag)
    {
        send_http_request(id, host_addr, host_port);

        const int interval = (1 + (rand() % 10)) * 200;
        cout << "Thread " << id << " going to sleep for " << interval << " seconds" << endl;
        this_thread::sleep_for(chrono::milliseconds(interval));
    }
}

DosClient::DosClient(int id, string host_addr, int host_port) :
    m_id(id),
    m_runner(do_dos, id, host_addr, host_port)
{
    cout << "New DoS client created for " << host_addr << endl;
}

void DosClient::stop()
{
    cout << "Stopping client " << m_id << "..." << endl;
    exit_thread_flag = true;
    m_runner.join();
    cout << "Client " << m_id << " now stopped" << endl;
}
