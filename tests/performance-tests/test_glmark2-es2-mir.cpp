#include "mir/default_server_configuration.h"
#include "mir_test/popen.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>

#include <stdlib.h>
#include <stdio.h>

#include <boost/regex.hpp>
#include <iostream>
#include <fstream>
#include <string>

class GLMark2Test : public mir_test_framework::InProcessServer
{
public:
    char const* argv[1] = {""}; 
    int argc = sizeof(argv) / sizeof(argv[0]);
    mir::DefaultServerConfiguration config{argc, argv};

    mir::DefaultServerConfiguration& server_config() override {return config;};
protected:
    enum ResultFileType {raw, json};
    virtual void RunGLMark2(char const* output_filename, ResultFileType file_type)
    {
        std::string const cmd = "glmark2-es2-mir --fullscreen";
        mir::test::Popen p(cmd);
       
        boost::cmatch matches;
        boost::regex re_glmark2_score(".*glmark2\\s+Score:\\s+(\\d+).*"); 
        std::string line;
        std::ofstream glmark2_output;
        glmark2_output.open(output_filename);        
        while (p.get_line(line))
        {
            if (file_type == raw)
            {
                glmark2_output << line; 
            }
            else if (file_type == json)
            {
                if (boost::regex_match(line.c_str(), matches, re_glmark2_score))
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
    }
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    unsetenv("MIR_SERVER_NO_FILE");
    RunGLMark2("glmark2_fullscreen_default.json", json);
}
