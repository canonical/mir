#include "mir_test_framework/command_line_server_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/options/default_configuration.h"

#include "mir_test/popen.h"
#include "mir_test_framework/in_process_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <regex.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>

namespace mo = mir::options;
namespace mtf = mir_test_framework;

namespace
{
class GLMark2Test : public mir_test_framework::InProcessServer
{
public:
 GLMark2Test()
        : config(mtf::configuration_from_commandline())
    {
    }

    mir::DefaultServerConfiguration& server_config() override {return config;};

protected:
    enum ResultFileType {raw, json};
    virtual void run_glmark2(char const* output_filename, ResultFileType file_type)
    {
        auto const cmd = "MIR_SOCKET=" + new_connection() + " glmark2-es2-mir --fullscreen";
        mir::test::Popen p(cmd);
       
        regex_t re_glmark2_score;
        int err = regcomp(&re_glmark2_score, ".*glmark2\\s+Score:\\s+(\\d+).*", 0);
        ASSERT_EQ(0, err);

        std::string line;
        std::ofstream glmark2_output;
        std::string score;
        glmark2_output.open(output_filename);
        while (p.get_line(line))
        {
            regmatch_t matches[2];
            if (!regexec(&re_glmark2_score, line.c_str(), 2, matches, 0)
                && score.length() == 0)
            {
                score = line.substr(matches[1].rm_so,
                                    matches[1].rm_eo - matches[1].rm_so);
            }

            if (file_type == raw)
            {
                glmark2_output << line << std::endl;
            }
        }
        regfree(&re_glmark2_score);
        
        ASSERT_THAT(score.length(), ::testing::Gt(0u));

        auto const minimum_acceptable_score = 52;
        EXPECT_THAT(stoi(score), ::testing::Ge(minimum_acceptable_score));

        if (file_type == json)
        {
            std::string json =  "{";
                json += "\"benchmark_name\":\"glmark2-es2-mir\"";
                json += ",";
                json += "\"score\":\"" + score + "\"";
                json += "}";
            glmark2_output << json;
        }
    }

private:
    mir::DefaultServerConfiguration config;
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    run_glmark2("/tmp/glmark2_fullscreen_default.results", raw);
}
}
