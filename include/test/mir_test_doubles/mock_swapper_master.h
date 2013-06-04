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

#include "src/server/compositor/buffer_swapper_master.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSwapperMaster : public compositor::BufferSwapperMaster
{
public:
    MockSwapperMaster()
    {}
    ~MockSwapperMaster() noexcept
    {}

    MOCK_METHOD0(client_acquire,     std::shared_ptr<compositor::Buffer>());
    MOCK_METHOD1(client_release,     void(std::shared_ptr<compositor::Buffer> const&));
    MOCK_METHOD0(compositor_acquire, std::shared_ptr<compositor::Buffer>());
    MOCK_METHOD1(compositor_release, void(std::shared_ptr<compositor::Buffer> const&));
    MOCK_METHOD0(force_client_completion, void());
    MOCK_METHOD2(end_responsibility, void(std::vector<std::shared_ptr<compositor::Buffer>>&, size_t&));
    MOCK_METHOD1(adopt_buffer, void(std::shared_ptr<compositor::Buffer> const&));
    MOCK_METHOD1(change_swapper, void(std::function<std::shared_ptr<compositor::BufferSwapper>
                                         (std::vector<std::shared_ptr<compositor::Buffer>>&, size_t&)>));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SWAPPER_MASTER_H_ */
