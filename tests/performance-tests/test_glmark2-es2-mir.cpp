#include <gtest/gtest.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <regex>
#include <string>

class GLMark2Test : public ::testing::Test
{
protected:
  virtual void RunGLMark2(const char * output_filename, bool bypass_enabled)
  {
    FILE * in;
    char buf[256];
    const char *cmd = "glmark2-es2-mir --fullscreen";
    const char *initial_bypass_state = getenv("MIR_BYPASS");
    std::ofstream glmark2_output;

    if(bypass_enabled)
        putenv(strdup("MIR_BYPASS=1")); 
    else
        putenv(strdup("MIR_BYPASS=0"));

    ASSERT_TRUE((in = popen(cmd, "r")));
    
    glmark2_output.open(output_filename);
    while(fgets(buf, sizeof(buf), in)!=NULL)
    {
        glmark2_output << buf;
    }
    glmark2_output.close();
    setenv("MIR_BYPASS", initial_bypass_state, 1);
  }
};

TEST_F(GLMark2Test, benchmark_with_bypass_enabled)
{
    RunGLMark2("glmark2_output_bypass_enabled.results", true);
}

TEST_F(GLMark2Test, benchmark_with_bypass_disabled)
{
    RunGLMark2("glmark2_output_bypass_disabled.results", false);
}
