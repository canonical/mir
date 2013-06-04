
#include <iostream>
#include <sstream>
#include <thread>

#include "threading.h"
#include "client.h"
#include "results.h"

std::string get_application_unique_name()
{
    std::stringstream strstr;
    strstr << std::this_thread::get_id();
    return strstr.str();
}

ThreadResults run_mir_test(std::chrono::seconds for_seconds)
{
    std::string thread_name(get_application_unique_name());
    ThreadResults results(thread_name);
    auto start_time = std::chrono::steady_clock::now();
    while (start_time + for_seconds > std::chrono::steady_clock::now())
    {
        ClientStateMachine::Ptr p = ClientStateMachine::Create();
        // std::cout << "Connecting" << std::endl;
        // std::cout << "C";
        if (!p->connect(thread_name))
        {
            results.addTestResult(false);
        }
        else
        {
            // std::cout << "S";
            if (!p->create_surface())
            {
                results.addTestResult(false);
            }
            else
            {
                // std::cout << "s";
                // continue test.
                p->release_surface();
            }
            // std::cout << "c";
            p->disconnect();
            results.addTestResult(true);
        }
        // std::cout << "Disconnecting" << std::endl;
        // std:: cout << ".";
    }
    return results;
}
