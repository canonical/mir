/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TESTING_CLIENT_CONFIGURATION
#define MIR_TESTING_CLIENT_CONFIGURATION

#include "mir/options/option.h"

namespace mir_test_framework
{

struct TestingClientConfiguration
{
    virtual ~TestingClientConfiguration() = default;

    // Code to run in client process
    virtual void exec() = 0;

    //clients respect the tests-use-real-graphics option by default. use
    //this function to force the use of the default client configuraiton
    virtual bool use_real_graphics(mir::options::Option const& options)
    {
        return options.get("tests-use-real-graphics", false);
    } 
};

}
#endif /* MIR_TESTING_CLIENT_CONFIGURATION */
