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

#ifndef MIR_PLATFORM_WAYLAND_CURSOR_H_
#define MIR_PLATFORM_WAYLAND_CURSOR_H_

#include <mir/graphics/cursor.h>
#include <mir/graphics/cursor_image.h>

#include <wayland-client.h>

#include <functional>
#include <mutex>

namespace mir
{
namespace platform
{
namespace wayland
{

class Cursor : public graphics::Cursor
{
public:
    Cursor(wl_compositor* compositor, wl_shm* shm, std::function<void()> flush_wl);

    ~Cursor();

    void show(std::shared_ptr<graphics::CursorImage> const& image) override;

    void hide() override;

    void move_to(geometry::Point position) override;

    void enter(wl_pointer* pointer);
    void leave(wl_pointer* pointer);

    void set_scale(float) override;

private:
    wl_shm* const shm;
    std::function<void()> const flush_wl;
    wl_surface* surface;

    std::recursive_mutex mutable mutex;
    wl_buffer* buffer{nullptr};
    wl_pointer* pointer{nullptr};

    std::shared_ptr<mir::graphics::CursorImage> current_cursor_image;
    float current_scale{1.0};
};
}
}
}

#endif //MIR_PLATFORM_WAYLAND_CURSOR_H_
