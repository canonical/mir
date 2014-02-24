#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <libgen.h>

#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std;

namespace
{
enum DescriptorType
{
    test_case,
    test_suite
};

DescriptorType check_line_for_test_case_or_suite(const string& line)
{
    if (line.find("  ") == 0)
        return test_case;

    return test_suite;
}

int get_output_width()
{
    const int fd_out{fileno(stdout)};
    const int max_width{65535};

    int width{max_width};

    if (isatty(fd_out))
    {
        struct winsize w;
        if (ioctl(fd_out, TIOCGWINSZ, &w) != -1)
            width = w.ws_col;
    }

    return width;
}

string ordinary_cmd_line_pattern()
{
    static const char* pattern = "ADD_TEST(\"%s.%s\" \"%s\" \"--gtest_filter=%s\")\n";
    return pattern;
}

vector<string> valgrind_cmd_patterns(vector<string> const& suppressions)
{
    vector<string> patterns{
        "valgrind",
        "--error-exitcode=1",
        "--trace-children=yes"
    };

    for (auto const& sup : suppressions)
        patterns.push_back(std::string("--suppressions=") + sup);

    vector<string> gtest_patterns{
        "%s",
        "--gtest_death_test_use_fork",
        "--gtest_filter=%s"
    };

    patterns.insert(patterns.end(), gtest_patterns.begin(), gtest_patterns.end());

    return patterns;
}

string memcheck_cmd_line_pattern(vector<string> const& suppressions)
{
    stringstream ss;

    ss << "ADD_TEST(\"memcheck(%s.%s)\"";
    for (auto& s : valgrind_cmd_patterns(suppressions))
        ss << " \"" << s << "\"";
    ss << ")" << endl;

    return ss.str();
}

std::string elide_string_left(const std::string& in, std::size_t max_size)
{
    assert(max_size >= 3);

    if (in.size() <= max_size)
        return in;

    std::string result(in.begin() + (in.size() - max_size), in.end());

    *(result.begin()) = '.';
    *(result.begin()+1) = '.';
    *(result.begin()+2) = '.';

    return result;
}

struct Configuration
{
    Configuration() : executable(NULL),
                      enable_memcheck(false),
                      memcheck_test(false)
    {
    }

    const char* executable;
    bool enable_memcheck;
    bool memcheck_test;
    std::vector<std::pair<std::string, std::string>> extra_environment;
    std::vector<std::string> suppressions;
};

bool parse_configuration_from_cmd_line(int argc, char** argv, Configuration& config)
{
    static struct option long_options[] = {
        {"executable", required_argument, 0, 0},
        {"enable-memcheck", no_argument, 0, 0},
        {"memcheck-test", no_argument, 0, 0},
        {"add-environment", required_argument, 0, 0},
        {"suppressions", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while(1)
    {
        int option_index = -1;
        const char *optname = "";
        int c = getopt_long(
            argc,
            argv,
            "e:m",
            long_options,
            &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        /* Detect an error in the passed options */
        if (c == ':' || c == '?')
            return false;

        /* Check if we got a long option and get its name */
        if (option_index != -1)
            optname = long_options[option_index].name;

        /* Handle options */
        if (c == 'e' || !strcmp(optname, "executable"))
            config.executable = optarg;
        else if (c == 'm' || !strcmp(optname, "enable-memcheck"))
            config.enable_memcheck = true;
        else if (!strcmp(optname, "memcheck-test"))
            config.memcheck_test = true;
        else if (!strcmp(optname, "add-environment"))
        {
            char const* equal_pos = strchr(optarg, '=');
            if (!equal_pos)
                return false;
            config.extra_environment.push_back(std::make_pair(std::string(optarg, equal_pos - optarg), std::string(equal_pos + 1)));
        }
        else if (!strcmp(optname, "suppressions"))
        {
            config.suppressions.push_back(std::string(optarg));
        }
    }

    return true;
}

string prepareMemcheckTestLine(string const& exe, vector<string> const& suppressions)
{
    stringstream ss;

    ss << "ADD_TEST(\"memcheck-test\" \"sh\" \"-c\" \"";
    for (auto& s : valgrind_cmd_patterns(suppressions))
        ss << s << " ";
    ss << "; if [ $? != 0 ]; then exit 0; else exit 1; fi\")";

    char cmd_line[1024] = "";
    snprintf(cmd_line,
             sizeof(cmd_line),
             ss.str().c_str(),
             exe.c_str(),
             "*"
             );

    return cmd_line;
}

void emitMemcheckTest(string const& exe, vector<string> const& suppressions)
{
    ifstream CTestTestfile("CTestTestfile.cmake", ifstream::in);
    bool need_memcheck_test = true;
    string line;

    string memcheckTestLine = prepareMemcheckTestLine(exe, suppressions);

    if (CTestTestfile.is_open())
    {
        while (CTestTestfile.good())
        {
            getline(CTestTestfile, line);

            if (line == memcheckTestLine)
                need_memcheck_test = false;
        }

        CTestTestfile.close();
    }

    if (need_memcheck_test)
    {
        ofstream CTestTestfileW ("CTestTestfile.cmake", ofstream::app | ofstream::out);

        if (CTestTestfileW.is_open())
        {
            CTestTestfileW << memcheckTestLine << endl;
            CTestTestfileW.close();
        }
    }
}
}

int main (int argc, char **argv)
{
    int output_width = get_output_width();

    cin >> noskipws;

    Configuration config;
    if (!parse_configuration_from_cmd_line(argc, argv, config) || config.executable == NULL)
    {
        cout << "Usage: PATH_TO_TEST_BINARY --gtest_list_tests | " << basename(argv[0])
             << " --executable PATH_TO_TEST_BINARY [--enable-memcheck]" << std::endl
             << " or " << std::endl << basename(argv[0])
             << " --executable PATH_TO_MEMCHECK_BINARY --memcheck-test" << std::endl;
        return 1;
    }

    if (config.memcheck_test)
    {
        emitMemcheckTest(config.executable, config.suppressions);
        return 0;
    }

    set<string> tests;
    string line;
    string current_test;

    while (getline (cin, line))
    {
        switch(check_line_for_test_case_or_suite(line))
        {
            case test_case:
                tests.insert(current_test + "*");
                break;
            case test_suite:
                current_test = line;
                break;
        }
    }

    ofstream testfilecmake;
    char* executable_copy = strdup(config.executable);
    string test_suite(basename(executable_copy));
    free(executable_copy);

    testfilecmake.open(string(test_suite  + "_test.cmake").c_str(), ios::out | ios::trunc);
    if (testfilecmake.is_open())
    {
        for (auto& env_pair : config.extra_environment)
        {
            testfilecmake << "SET( ENV{"<<env_pair.first<<"} \""<<env_pair.second<<"\" )"<<std::endl;
        }
        for (auto test = tests.begin(); test != tests.end(); ++ test)
        {
            static char cmd_line[1024] = "";
            snprintf(
                cmd_line,
                sizeof(cmd_line),
                config.enable_memcheck ? memcheck_cmd_line_pattern(config.suppressions).c_str() :
                                         ordinary_cmd_line_pattern().c_str(),
                test_suite.c_str(),
                elide_string_left(*test, output_width/2).c_str(),
                config.executable,
                test->c_str());

            if (testfilecmake.good())
            {
                testfilecmake << cmd_line;
            }
        }

        testfilecmake.close();
    }

    ifstream CTestTestfile("CTestTestfile.cmake", ifstream::in);
    bool need_include = true;
    line.clear();

    string includeLine = string ("INCLUDE (") +
                         test_suite  +
                         string ("_test.cmake)");

    if (CTestTestfile.is_open())
    {
        while (CTestTestfile.good())
        {
            getline(CTestTestfile, line);

            if (line == includeLine)
                need_include = false;
        }

        CTestTestfile.close();
    }

    if (need_include)
    {
        ofstream CTestTestfileW ("CTestTestfile.cmake", ofstream::app | ofstream::out);

        if (CTestTestfileW.is_open())
        {
            CTestTestfileW << includeLine << endl;
            CTestTestfileW.close();
        }
    }

    return 0;
}
