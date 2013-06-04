#include "results.h"

#include <sstream>

ThreadResults::ThreadResults(std::string thread_id)
: thread_id_(thread_id)
, tests_passed_(0)
, tests_failed_(0)
{}


void ThreadResults::addTestResult(bool passed)
{
    if (passed)
        ++tests_passed_;
    else
        ++tests_failed_;
}


std::string ThreadResults::summary() const
{
    std::stringstream strstr;
    strstr << "Thread '" << thread_id_ << "' ran "
        << tests_passed_ + tests_failed_ << " tests, of which "
        << tests_passed_ << " passed and "
        << tests_failed_ << " failed." << std::endl;
    return strstr.str();
}
