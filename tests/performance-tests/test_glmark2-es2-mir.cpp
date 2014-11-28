#include "mir_test_framework/command_line_server_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/options/default_configuration.h"

#include "mir_test/popen.h"
#include "mir_test_framework/in_process_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    mir::DefaultServerConfiguration& server_config() override
    {
#ifdef ANDROID
        /*
         * Workaround instability that causes freezes when combining
         * Android overlays with high-speed frame dropping (LP: #1391261).
         * Fortunately glmark2 is the only use case that exploits this issue
         * right now.
         */
        setenv("MIR_SERVER_DISABLE_OVERLAYS", "true", 1);
#endif
        return config;
    };

protected:
    enum ResultFileType {raw, json};
    virtual void run_glmark2(char const* output_filename, ResultFileType file_type)
    {
        auto const cmd = "MIR_SOCKET=" + new_connection() + " glmark2-es2-mir --fullscreen";
        mir::test::Popen p(cmd);
       
        std::string line;
        std::ofstream glmark2_output;
        int score = -1;
        glmark2_output.open(output_filename);
        while (p.get_line(line))
        {
            int match;
            if (sscanf(line.c_str(), " glmark2 Score: %d", &match) == 1)
            {
                score = match;
            }

            if (file_type == raw)
            {
                glmark2_output << line << std::endl;
            }
        }
        
        auto const minimum_acceptable_score = 52;
        EXPECT_THAT(score, ::testing::Ge(minimum_acceptable_score));

        if (file_type == json)
        {
            std::string json =  "{";
                json += "\"benchmark_name\":\"glmark2-es2-mir\"";
                json += ",";
                json += "\"score\":\"" + std::to_string(score) + "\"";
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
