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
#ifndef MIR_TEST_DOUBLES_MOCK_SWAPPER_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_SWAPPER_FACTORY_H_

#include "mir/compositor/buffer_allocation_strategy.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockSwapperFactory : public compositor::BufferAllocationStrategy
{
public:
    ~MockSwapperFactory() noexcept {}

    MOCK_CONST_METHOD4(create_swapper_reuse_buffers,
        std::shared_ptr<compositor::BufferSwapper>(graphics::BufferProperties const&,
            std::vector<std::shared_ptr<graphics::Buffer>>&, size_t, compositor::SwapperType));
    MOCK_CONST_METHOD3(create_swapper_new_buffers,
        std::shared_ptr<compositor::BufferSwapper>(
            graphics::BufferProperties&, graphics::BufferProperties const&, compositor::SwapperType));
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SWAPPER_FACTORY_H_ */
