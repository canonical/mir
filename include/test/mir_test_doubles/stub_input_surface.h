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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_SURFACE_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_SURFACE_H_

#include "mir/input/surface.h"

#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubInputSurface : public mir::input::Surface
{
    StubInputSurface(std::shared_ptr<mir::input::InputChannel> const& channel)
        : channel(channel)
    {
    }
    StubInputSurface()
        : channel(nullptr)
    {
    }

    std::shared_ptr<mir::input::InputChannel> input_channel() const
    {
        return channel;
    }

    mir::input::InputReceptionMode reception_mode() const { return mir::input::InputReceptionMode::normal; }
    void consume(MirEvent const&) override  {}
    std::string name() const { return {}; }
    mir::geometry::Rectangle input_bounds() const override { return {{},{}}; }
    bool input_area_contains(mir::geometry::Point const&) const { return false; }
    
    std::shared_ptr<graphics::CursorImage> cursor_image() const { return nullptr; }

    std::shared_ptr<mir::input::InputChannel> const channel;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_INPUT_SURFACE_H_ */
