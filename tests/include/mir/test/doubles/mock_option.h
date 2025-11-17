/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    MOCK_METHOD(bool, is_set, (char const*), (const, override));
    MOCK_METHOD(bool, get, (char const*, bool), (const, override));
    MOCK_METHOD(int, get, (char const*, int), (const, override));
    MOCK_METHOD(std::string, get, (char const* name, char const*), (const, override));
    MOCK_METHOD(boost::any const&, get, (char const*), (const, override));
};

}
}
}

#endif
