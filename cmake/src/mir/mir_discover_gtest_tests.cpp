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
    char *base = basename (argv[1]);
    string   gtestName (base);

    testfilecmake.open(string(gtestName  + "_test.cmake").c_str(), ios::out | ios::trunc);

    if (testfilecmake.is_open())
    {
    	for (auto jt: tests)
        {
			if (testfilecmake.good())
			{
				static string const add_test("ADD_TEST (");
				string test_exec(" \"" + string (argv[1]) + "\"");
				static string const gtest_filter("\"--gtest_filter=");
				static string const filter_begin ("");
				static string const filter_end ("\")");

				testfilecmake << add_test << gtestName << '.' << jt << test_exec << gtest_filter << filter_begin << jt << filter_end << endl;
			}
        }

        testfilecmake.close ();
    }

    ifstream CTestTestfile ("CTestTestfile.cmake", ifstream::in);
    bool needsInclude = true;
    line.clear ();

    string includeLine = string ("INCLUDE (") +
                         gtestName  +
                         string ("_test.cmake)");

    if (CTestTestfile.is_open ())
    {
        while (CTestTestfile.good ())
        {
            getline (CTestTestfile, line);

            if (line == includeLine)
                needsInclude = false;
        }

        CTestTestfile.close ();
    }

    if (needsInclude)
    {
        ofstream CTestTestfileW ("CTestTestfile.cmake", ofstream::app | ofstream::out);

        if (CTestTestfileW.is_open ())
        {
            CTestTestfileW << includeLine << endl;
            CTestTestfileW.close ();
        }
    }

    return 0;
}
