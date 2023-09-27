/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
*
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_VIRTUAL_DISPLAY_BUFFER_H
#define MIR_GRAPHICS_VIRTUAL_DISPLAY_BUFFER_H

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderable.h"

namespace mir
{
namespace graphics
{
namespace virt
{
class DisplayBuffer : public mir::graphics::DisplayBuffer,
    public graphics::DisplaySyncGroup
{
public:
    DisplayBuffer(std::shared_ptr<DisplayInterfaceProvider> provider);

    geometry::Rectangle view_area() const override;
    bool overlay(std::vector<DisplayElement> const& renderlist) override;
    glm::mat2 transformation() const override;
    void set_next_image(std::unique_ptr<Framebuffer> content) override;
    auto display_provider() const -> std::shared_ptr<DisplayInterfaceProvider> override;

    void for_each_display_buffer(const std::function<void(mir::graphics::DisplayBuffer &)> &f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

private:
    geometry::Rectangle area;
    glm::mat2 transform;
    std::shared_ptr<DisplayInterfaceProvider> provider;
};
}
}
}

#endif //MIR_GRAPHICS_VIRTUAL_DISPLAY_BUFFER_H
