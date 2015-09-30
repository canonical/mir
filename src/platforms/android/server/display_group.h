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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_GROUP_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_GROUP_H_

#include "mir_toolkit/common.h"
#include "mir/graphics/display.h"
#include "mir/geometry/displacement.h"
#include "display_name.h"
#include <map>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace android
{
class ConfigurableDisplayBuffer;
class DisplayDevice;

class DisplayGroup : public graphics::DisplaySyncGroup
{
public:
    DisplayGroup(
        std::shared_ptr<DisplayDevice> const& device,
        std::unique_ptr<ConfigurableDisplayBuffer> primary_buffer);

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    void add(DisplayName name, std::unique_ptr<ConfigurableDisplayBuffer> buffer);
    void remove(DisplayName name);
    void configure(DisplayName name, MirPowerMode, MirOrientation, geometry::Displacement);
    bool display_present(DisplayName name) const;

private:
    std::mutex mutable guard;
    std::shared_ptr<DisplayDevice> const device;
    std::map<DisplayName, std::unique_ptr<ConfigurableDisplayBuffer>> dbs;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_GROUP_H_ */
