// mir-stress.cpp - a stress test client for mir.
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <future>

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "threading.h"
#include "results.h"

const std::string option_str("hn:t:p");
const std::string usage("Usage:\n"
    "   -h:         This help text.\n"
    "   -n seconds  Number of seconds to run. Default is 600 (10 minutes).\n"
    "   -t threads  Number of threads to create. Default is the number of cores\n"
    "               on this machine.\n"
    );


int main(int argc, char **argv)
{
    std::chrono::seconds duration_to_run(60 * 10);
    unsigned int num_threads = std::thread::hardware_concurrency();
    int arg;
    opterr = 0;
    while ((arg = getopt(argc, argv, option_str.c_str())) != -1)
    {
        switch (arg)
        {
        case 'n':
            duration_to_run = std::chrono::seconds(std::atoi(optarg));
            break;

        case 't':
            // TODO: limit to sensible values.
            num_threads = std::atoi(optarg);
            break;
        case '?':
        case 'h':
        default:
            std::cout << usage << std::endl;
            return -1;
        }
    }

    std::vector<std::future<ThreadResults>> futures;
    for (int i = 0; i < num_threads; i++)
    {
        std::cout << "Creating thread..." << std::endl;
        futures.push_back(
            std::async(
                std::launch::async,
                run_mir_test,
                duration_to_run
                )
            );
    }
    std::vector<ThreadResults> results;
    for (auto &t: futures)
    {
        results.push_back(t.get());
    }
    for (auto &r: results)
    {
        std::cout << r.summary();
    }
    return 0;
}
