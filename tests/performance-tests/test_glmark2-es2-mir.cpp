#include <gtest/gtest.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <regex>
#include <string>

class GLMark2Test : public ::testing::Test
{
protected:
  virtual void RunGLMark2(const char * output_filename)
  {
    FILE * in;
    char buf[256];
    const char *cmd = "glmark2-es2-mir --fullscreen";
    std::ofstream glmark2_output;

    ASSERT_TRUE((in = popen(cmd, "r")));
    
    glmark2_output.open(output_filename);
    while(fgets(buf, sizeof(buf), in)!=NULL)
    {
        glmark2_output << buf;
    }
    glmark2_output.close();
  }
};

TEST_F(GLMark2Test, benchmark_fullscreen_default)
{
    RunGLMark2("glmark2_fullscreen_default.results");
}
