#include "mir/default_server_configuration.h"
#include "mir/options/default_configuration.h"

#include "mir_test/popen.h"
#include "mir_test_framework/in_process_server.h"

#include <gtest/gtest.h>

#include <stdlib.h>
#include <stdio.h>

#include <boost/regex.hpp>
#include <fstream>
#include <string>

namespace
{
// FIXME remove this when support for passing cli options to 
// DefaultServerConfiguration is implemented
char const* dummy[] = {0};
int argc = 0;
char const** argv = dummy;

class GLMark2Test : public mir_test_framework::InProcessServer
{
public:
 GLMark2Test()
        : config(argc, argv)
    {
    }

    mir::DefaultServerConfiguration& server_config() override {return config;};

protected:
    enum ResultFileType {raw, json};
    virtual void run_glmark2(char const* output_filename, ResultFileType file_type)
    {
        auto const cmd = "MIR_SOCKET=" + new_connection() + " glmark2-es2-mir --fullscreen";
        mir::test::Popen p(cmd);
       
        boost::cmatch matches;
        boost::regex re_glmark2_score(".*glmark2\\s+Score:\\s+(\\d+).*");
        std::string line;
        std::ofstream glmark2_output;
        std::string score = "";
        glmark2_output.open(output_filename);
        while (p.get_line(line))
        {
            if(boost::regex_match(line.c_str(), matches, re_glmark2_score)
                && score.length() == 0)
            {
                score = matches[1];
            }

            if (file_type == raw)
            {
                glmark2_output << line;
            }
        }

        ASSERT_GT(score.length(), 0u);
        EXPECT_GT(atoi(score.c_str()), 52);

        if (file_type == json)
        {
            std::string json =  "{";
                json += "'benchmark_name':'glmark2-es2-mir'";
                json += ",";
                json += "'score':'" + score + "'";
                json += "}";
            glmark2_output << json;
        }
    }

private:
    mir::DefaultServerConfiguration config;
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    run_glmark2("glmark2_fullscreen_default.json", json);
}
}
