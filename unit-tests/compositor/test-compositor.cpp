/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;

namespace
{

class mock_buffer_texture_binder : public mc::buffer_texture_binder
{
public:
    MOCK_METHOD0(bind_buffer_to_texture, void ());
};
}


TEST(compositor, render)
{
    using namespace testing;

    mock_buffer_texture_binder buffer_texture_binder;
    mc::compositor comp(nullptr, &buffer_texture_binder);

    EXPECT_CALL(buffer_texture_binder, bind_buffer_to_texture()).Times(AtLeast(1));

    comp.render(nullptr);
}
