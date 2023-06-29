#include <atomic>
#include <iostream>

#include "dos_server.h"

using namespace std;

static atomic<bool> exit_thread_flag{false};

static void do_serve()
{
    while (!exit_thread_flag)
    {
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
