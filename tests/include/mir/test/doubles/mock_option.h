/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_OPTION_H_
#define MIR_TEST_DOUBLES_MOCK_OPTION_H_

#include "mir/options/option.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockOption : mir::options::Option
{
    MOCK_CONST_METHOD1(is_set, bool(char const*));
    MOCK_CONST_METHOD2(get, bool(char const*, bool));
    MOCK_CONST_METHOD2(get, int(char const*, int));
    MOCK_CONST_METHOD2(get, std::string(char const* name, char const*));
    MOCK_CONST_METHOD1(get, boost::any const&(char const*));
};

}
}
}

#endif
