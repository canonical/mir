/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/temporary_environment_value.h"

#include <cstdlib>

namespace mtf = mir_test_framework;

mtf::TemporaryEnvironmentValue::TemporaryEnvironmentValue(char const* name, char const* value) :
    name{name},
    has_old_value{getenv(name) != nullptr},
    old_value{has_old_value ? getenv(name) : ""}
{
    if (value)
        setenv(name, value, overwrite);
    else
        unsetenv(name);
}

mtf::TemporaryEnvironmentValue::~TemporaryEnvironmentValue()
{
    if (has_old_value)
        setenv(name.c_str(), old_value.c_str(), overwrite);
    else
        unsetenv(name.c_str());
}
