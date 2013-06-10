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
#ifndef MIR_TEST_DOUBLES_MOCK_SWAPPER_MASTER_H_
#define MIR_TEST_DOUBLES_MOCK_SWAPPER_MASTER_H_

#include "src/server/compositor/swapper_director.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSwapperDirector : public compositor::SwapperDirector
{
public:
    MockSwapperDirector()
    {}
    ~MockSwapperDirector() noexcept
    {}

    MOCK_METHOD0(client_acquire,     std::shared_ptr<compositor::Buffer>());
    MOCK_METHOD1(client_release,     void(std::shared_ptr<compositor::Buffer> const&));
    MOCK_METHOD0(compositor_acquire, std::shared_ptr<compositor::Buffer>());
    MOCK_METHOD1(compositor_release, void(std::shared_ptr<compositor::Buffer> const&));
    MOCK_METHOD1(allow_framedropping, void(bool));
    MOCK_CONST_METHOD0(properties, compositor::BufferProperties());
    MOCK_METHOD0(shutdown, void());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SWAPPER_MASTER_H_ */
