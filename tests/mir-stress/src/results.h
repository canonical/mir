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

#ifndef MIR_STRESS_TEST_RESULTS_H_
#define MIR_STRESS_TEST_RESULTS_H_

#include <string>

class ThreadResults
{
public:
    ThreadResults(std::string thread_id);
    void addTestResult(bool passed);

    std::string summary() const;
    bool success() const;
private:
    std::string thread_id_;
    int tests_passed_;
    int tests_failed_;
};

#endif
