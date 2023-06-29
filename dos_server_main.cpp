#include <iostream>

#include "dos_server.h"

using namespace std;

int main(void)
{
    auto server = make_unique<DosServer>();

    do
    {
        cout << "DoS server now running...\nHit Enter to quit" << endl;
    } while (cin.get() != '\n');

    server->stop();

    return 0;
}
