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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_FD_MATCHER_H_
#define MIR_TEST_DOUBLES_FD_MATCHER_H_

#include "mir/fd.h"
#include <unistd.h>
#include <fcntl.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
MATCHER_P(RawFdMatcher, value, std::string("raw_fd does not match mir::Fd"))
{
    return value == arg; 
}
MATCHER(RawFdIsValid, std::string("raw_fd is not valid"))
{
    return (fcntl(arg, F_GETFD) != -1);
}
}
}
}

#endif /* MIR_TEST_DOUBLES_FD_MATCHER_H_ */
