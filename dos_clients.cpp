#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>

#include "dos_client.h"

using namespace std;

static const int MAX_NUMBER_OF_CLIENTS = 10;

static vector<unique_ptr<DosClient>> run_clients(int num_of_clients)
{
    cout << "Starting all clients now..." << endl;
    vector<unique_ptr<DosClient>> dos_clients;
    for (int i = 0; i < num_of_clients; i++)
    {
        dos_clients.push_back(make_unique<DosClient>(i, "127.0.0.1", 8000));
    }
    return dos_clients;
}

static void stop_clients(vector<unique_ptr<DosClient>> &dos_clients)
{
    cout << "Stopping all clients now..." << endl;
    for (auto &client : dos_clients)
    {
        client->stop();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: dos_clients <num_of_clients>" << endl;
        return -1;
    }

    int num_of_clients = 0;

    try
    {
        num_of_clients = stoi(argv[1]);
    }
    catch(invalid_argument const &ex)
    {
        cout << "Invalid num_of_clients argument" << endl;
        return -2;
    }

    if (num_of_clients <= 0 || num_of_clients > MAX_NUMBER_OF_CLIENTS)
    {
        cout << "Illegal number of clients requested" << endl;
        return -3;
    }

    auto dos_clients = run_clients(num_of_clients);

    do
    {
        cout << dos_clients.size() << " DoS clients now running...\nHit Enter to quit" << endl;
    } while (cin.get() != '\n');

    stop_clients(dos_clients);

    return 0;
}
