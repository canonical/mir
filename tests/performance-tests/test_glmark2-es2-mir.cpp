#include "mir/default_server_configuration.h"
#include "mir/display_server.h"
#include "mir/run_mir.h"

#include <gtest/gtest.h>

#include <stdlib.h>
#include <stdio.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <boost/thread/thread.hpp> 
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <fstream>
#include <string>
#include <thread>

class GLMark2Test : public ::testing::Test
{
protected:
    enum ResultFileType {raw, json};
    mir::DisplayServer* result{nullptr};
    std::thread server_thread;

    mir::DisplayServer* launch_mir_server()
    {
        char const* argv[] = {""};
        int argc = sizeof(argv) / sizeof(argv[0]);
        mir::DefaultServerConfiguration config(argc, argv);

        std::condition_variable cv;
        std::mutex mutex;
        server_thread = std::thread([&]
        {
            try
            {
                run_mir(config, [&](mir::DisplayServer& ds) {
                    std::unique_lock<std::mutex> lock(mutex);
                    result = &ds;
                    cv.notify_one();
                });
            }
            catch(...)
            {
                mir::report_exception(std::cerr);
            }
        });
       
        using namespace std::chrono; 
        auto const time_limit = system_clock::now() + seconds(2);
        std::unique_lock<std::mutex> lock(mutex);
        while(!result)
        {
            cv.wait_until(lock, time_limit);
        }
 
        return result;
    }
    
    virtual void RunGLMark2(char const* output_filename, ResultFileType file_type)
    {
        mir::DisplayServer* ds = launch_mir_server();
        ASSERT_TRUE(ds);

        boost::cmatch matches;
        boost::regex re_glmark2_score(".*glmark2\\s+Score:\\s+(\\d+).*");
        FILE* in;
        char* line = NULL;
        size_t len = 0;
        ssize_t read;
        std::ofstream glmark2_output;
        glmark2_output.open(output_filename);

        char const* cmd = "glmark2-es2-mir -b texture --fullscreen";
        ASSERT_TRUE((in = popen(cmd, "r")));
        
        while ((read = getline(&line, &len, in)) != -1)
        {
            if (file_type == raw)
            {
                glmark2_output << line;
            }
            else if (file_type == json)
            {
                std::cout << line;
                if (boost::regex_match(line, matches, re_glmark2_score))
                {
                    std::string json =  "{";
                        json += "'benchmark_name':'glmark2-es2-mir'";
                        json += ",";
                        json += "'score':'" + matches[1] + "'";
                        json += "}";
                    glmark2_output << json;
                    break;
                }
            }
        }
        
        ds->stop();
        if (server_thread.joinable()) server_thread.join();
        free(line);
        fclose(in);
    }
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    RunGLMark2("glmark2_fullscreen_default.json", json);
}
