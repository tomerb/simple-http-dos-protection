#pragma once

#include <thread>
#include <vector>

#include "safe_msg_queue.h"

class DosServer
{
 public:
    struct RequestMsg
    {
        std::string request;
        int sockfd;
    };

    DosServer();
    ~DosServer();
    void stop();
 private:
    int m_sockfd;
    std::thread m_server;
    std::vector<std::thread> m_runners;

    SafeMsgQueue<RequestMsg> m_msg_queue;
};
