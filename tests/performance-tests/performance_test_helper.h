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
 * Authored by: Josh Arenson <joshua.arenson@canonical.com>
 */

#ifndef MIR_PERFORMANCE_TEST_HELPER_H_
#define MIR_PERFORMANCE_TEST_HELPER_H_

#include <stdlib.h>

#include <iostream>
#include <regex>
#include <string>

// A helper class to add PPAs and install packages for performance testing
class PerformanceTestHelper
{
public:
  virtual bool add_ppa(std::string ppa_name); // ppa:<ppa/name>
  virtual bool install_package(std::string package_name);

private:
  virtual ~PerformanceTestHelper() = default;
  PerformanceTestHelper() = default;
  bool package_is_installed(std::string package_name);
};

#endif // MIR_PERFORMANCE_TEST_HELPER_H_
