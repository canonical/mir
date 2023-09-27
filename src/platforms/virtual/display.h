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

#ifndef MIR_PLATFORMS_VIRTUAL_DISPLAY_H_
#define MIR_PLATFORMS_VIRTUAL_DISPLAY_H_

#include "display_buffer.h"
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
    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup &)> &f) override;
    std::unique_ptr<DisplayConfiguration> configuration() const override;
    bool apply_if_configuration_preserves_display_buffers(const DisplayConfiguration &conf) override;
    void configure(const DisplayConfiguration &conf) override;
    void register_configuration_change_handler(
        EventHandlerRegister &handlers,
       const DisplayConfigurationChangeHandler &conf_change_handler) override;
    void pause() override;
    void resume() override;
    std::shared_ptr<Cursor> create_hardware_cursor() override;

private:
    std::vector<std::unique_ptr<mir::graphics::virt::DisplayBuffer>> display_buffers;
};
}
}
}


#endif //MIR_PLATFORMS_VIRTUAL_DISPLAY_H_
