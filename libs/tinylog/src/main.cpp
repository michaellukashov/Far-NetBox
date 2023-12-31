#include <iostream>

#include "TinyLog.h"

using namespace std;

int main() {
    g_tinylog.SetLogLevel(Utils::INFO);

    string log = "hello world";

    cerr << "start time: " << Utils::GetCurrentTime() << endl;

    for (int i = 0; i < 10000000; i++)
    {
        LOG_INFO << log;
    }

    cerr << "end time: " << Utils::GetCurrentTime() << endl;
    cin.get();

    return 0;
}