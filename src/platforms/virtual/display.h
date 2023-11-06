/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_VIRT_DISPLAY_H_
#define MIR_GRAPHICS_VIRT_DISPLAY_H_

#include "platform.h"
#include <mir/graphics/display.h>

namespace mir
{
namespace graphics
{
namespace virt
{

class Display : public mir::graphics::Display
{
public:
    explicit Display(std::vector<VirtualOutputConfig> const& output_sizes);
    void for_each_display_sync_group(std::function<void(DisplaySyncGroup &)> const& f) override;
    std::unique_ptr<mir::graphics::DisplayConfiguration> configuration() const override;
    bool apply_if_configuration_preserves_display_buffers(mir::graphics::DisplayConfiguration const& conf) override;
    void configure(mir::graphics::DisplayConfiguration const& conf) override;
    void register_configuration_change_handler(
        EventHandlerRegister &handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;
    void pause() override;
    void resume() override;
    std::shared_ptr<Cursor> create_hardware_cursor() override;

private:
    std::mutex mutable mutex;
    std::shared_ptr<DisplayConfiguration> display_configuration;
};
}
}
}


#endif //MIR_GRAPHICS_VIRT_DISPLAY_H_
