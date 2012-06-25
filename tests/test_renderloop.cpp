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

#include "mir/graphics/framebuffer_backend.h"
#include "mir/compositor/drawer.h"
#include "mir/compositor/compositor.h"
#include "mir/compositor/buffer_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;


namespace
{
class mock_framebuffer_backend : public mg::framebuffer_backend
{
public:
    MOCK_METHOD0(render, void ());
};
}


TEST(compositor_renderloop, notify_sync_and_see_paint)
{
    using namespace testing;

    mock_framebuffer_backend graphics;
    mc::buffer_manager buffer_manager(&graphics);
    mc::drawer&& comp = mc::compositor(nullptr, &buffer_manager);

    EXPECT_CALL(graphics, render()).Times(AtLeast(1));

    comp.render(nullptr);
}
