/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_GROUP_H_
#define MIR_GRAPHICS_X_DISPLAY_GROUP_H_

#include "mir_toolkit/common.h"
#include "mir/graphics/display.h"
#include "display_buffer.h"

namespace mir
{
namespace graphics
{
namespace X
{

class DisplayGroup : public graphics::DisplaySyncGroup
{
public:
    DisplayGroup(std::unique_ptr<DisplayBuffer> primary_buffer);

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;
    void set_orientation(MirOrientation orientation);

private:
    std::unique_ptr<DisplayBuffer> display_buffer;
};

}
}
}

#endif /* MIR_GRAPHICS_X_DISPLAY_GROUP_H_ */
