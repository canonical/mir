#include <map>
#include <vector>
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

    map<string, vector<string> > testCases;
    string line;
    string currentTestCase;

    while (getline (cin, line))
    {
	/* Is test case */
	if (line.find ("  ") == 0)
	    testCases[currentTestCase].push_back (currentTestCase + line.substr (2));
	else
	    currentTestCase = line;

    }

    ofstream testfilecmake;
    char *base = basename (argv[1]);
    string   gtestName (base);

    testfilecmake.open (string (gtestName  + "_test.cmake").c_str (), ios::out | ios::trunc);

    if (testfilecmake.is_open ())
    {
	for (map <string, vector<string> >::iterator it = testCases.begin ();
	     it != testCases.end (); it++)
	{
	    for (vector <string>::iterator jt = it->second.begin ();
		 jt != it->second.end (); jt++)
	    {
		if (testfilecmake.good ())
		{
		    string addTest ("ADD_TEST (");
		    string testExec (" \"" + string (argv[1]) + "\"");
		    string gTestFilter ("\"--gtest_filter=\"");
		    string filterBegin ("\"");
		    string filterEnd ("\")");

		    testfilecmake << addTest << *jt << testExec << gTestFilter << filterBegin << *jt << filterEnd << endl;
		}
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
