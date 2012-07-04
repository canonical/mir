#include <map>
#include <set>
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <iterator>
#include <iostream>
#include <libgen.h>

using namespace std;

int main (int argc, char **argv)
{
    cin >> noskipws;

    if (argc < 2)
    {
        cout << "Usage: PATH_TO_TEST_BINARY --gtest_list_tests | ./build_test_cases PATH_TO_TEST_BINARY";
        return 1;
    }

    set<string> tests;
    string line;
    string current_test;

    while (getline (cin, line))
    {
        /* Is test case */
        if (line.find ("  ") == 0)
        {
            tests.insert(current_test + "*");
        }
        else
            current_test = line;
    }

    ofstream testfilecmake;
    char *base = basename(argv[1]);
    string   test_suite(base);

    testfilecmake.open(string(test_suite  + "_test.cmake").c_str(), ios::out | ios::trunc);

    if (testfilecmake.is_open())
    {
    	for (auto test: tests)
        {
			if (testfilecmake.good())
			{
				testfilecmake
					<< "ADD_TEST ("
					<< test_suite << '.' << test
					<< " \"" << argv[1] << "\""
					<< "\"--gtest_filter=" << test << "\")" << endl;
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
