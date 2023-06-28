#pragma once

#include <string>
#include <thread>

class DosClient
{
 public:
    DosClient(int id, std::string host_addr, int host_port);
    void stop();
 private:
    int m_id;
    std::thread m_runner;
};
