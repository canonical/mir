/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DOUBLES_MOCK_SWAPPER_H_
#define MIR_TEST_DOUBLES_MOCK_SWAPPER_H_

#include "mir/compositor/buffer_swapper.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSwapper : public compositor::BufferSwapper
{
public:
    MockSwapper(std::shared_ptr<compositor::Buffer> buffer)
        : default_buffer(buffer)
    {
        using namespace testing;

        ON_CALL(*this, compositor_acquire(_,_))
        .WillByDefault(SetArgReferee<0>(default_buffer));
        ON_CALL(*this, client_acquire(_,_))
        .WillByDefault(SetArgReferee<0>(default_buffer));
    };

    MOCK_METHOD2(client_acquire,     void(std::shared_ptr<compositor::Buffer>& buffer_reference, compositor::BufferID& dequeued_buffer));
    MOCK_METHOD1(client_release,     void(compositor::BufferID queued_buffer));
    MOCK_METHOD2(compositor_acquire, void(std::shared_ptr<compositor::Buffer>& buffer_reference, compositor::BufferID& dequeued_buffer));
    MOCK_METHOD1(compositor_release, void(compositor::BufferID queued_buffer));
    MOCK_METHOD0(shutdown, void());

private:
    std::shared_ptr<compositor::Buffer> default_buffer;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SWAPPER_H_ */
