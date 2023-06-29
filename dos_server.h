#pragma once

#include <thread>

class DosServer
{
 public:
    DosServer();
    void stop();
 private:
    std::thread m_runner;
};
