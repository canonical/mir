/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

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

bool ThreadResults::success() const
{
    return tests_failed_ == 0;
}
