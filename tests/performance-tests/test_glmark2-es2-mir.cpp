#include <gtest/gtest.h>

#include <stdlib.h>
#include <stdio.h>

#include <boost/regex.hpp>
#include <iostream>
#include <fstream>
#include <string>

class GLMark2Test : public ::testing::Test
{
protected:
    enum ResultFileType {raw, json};
    virtual void RunGLMark2(char const* output_filename, ResultFileType file_type)
    {
        boost::cmatch matches;
        boost::regex re_glmark2_score(".*glmark2\\s+Score:\\s+(\\d+).*");
        FILE* in;
        char* line = NULL;
        size_t len = 0;
        ssize_t read;
        std::ofstream glmark2_output;
        
        char const* cmd = "./mir_demo_server_shell&";
        in = popen(cmd, "r");
        
        cmd = "glmark2-es2-mir --fullscreen";
        ASSERT_TRUE((in = popen(cmd, "r")));
    
        glmark2_output.open(output_filename);
        while ((read = getline(&line, &len, in)) != -1)
        {
            if (file_type == raw)
            {
                glmark2_output << line; 
            }
            else if (file_type == json)
            {
                if (boost::regex_match(line, matches, re_glmark2_score))
                {
                std::string json =  "{";
                    json += "'benchmark_name':'glmark2-es2-mir'";
                    json += ",";
                    json += "'score':'" + matches[1] + "'";
                    json += "}";
                glmark2_output << json;
                }
            }
        }
    
        free(line);
        fclose(in);
    }
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    RunGLMark2("glmark2_fullscreen_default.json", json);
}
