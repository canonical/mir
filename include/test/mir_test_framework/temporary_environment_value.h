/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_FRAMEWORK_ENVIRONMENT_VALUE_H_
#define MIR_TEST_FRAMEWORK_ENVIRONMENT_VALUE_H_

#include <string>

namespace mir_test_framework
{
class TemporaryEnvironmentValue
{
public:
    TemporaryEnvironmentValue(char const* name, char const* value);

    ~TemporaryEnvironmentValue();

private:
    static int const overwrite = 1;
    std::string const name;
    bool const has_old_value;
    std::string const old_value;
};
}

#endif /* MIR_TEST_FRAMEWORK_ENVIRONMENT_VALUE_H_ */
