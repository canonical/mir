/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_H_

#include "null_display.h"
#include "stub_display_buffer.h"
#include "stub_display_configuration.h"

#include "mir/geometry/rectangle.h"

#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{

class StubDisplay : public NullDisplay
{
public:
    StubDisplay(std::vector<geometry::Rectangle> const& output_rects)
        : output_rects{output_rects}
    {
        for (auto const& output_rect : output_rects)
            display_buffers.emplace_back(output_rect);
    }

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override
    {
        for (auto& db : display_buffers)
            f(db);
    }

    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<graphics::DisplayConfiguration>(
            new StubDisplayConfig(output_rects)
        );
    }

    std::vector<geometry::Rectangle> const output_rects;
private:
    std::vector<StubDisplayBuffer> display_buffers;
};

}
}
}

#endif
