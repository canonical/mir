#ifndef RESULTS_H
#define RESULTS_H

#include <string>

class ThreadResults
{
public:
    ThreadResults(std::string thread_id);
    void addTestResult(bool passed);

    std::string summary() const;
private:
    std::string thread_id_;
    int tests_passed_;
    int tests_failed_;
};

#endif
