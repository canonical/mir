/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "relative_pointer_unstable_v1.h"
#include "relative-pointer-unstable-v1_wrapper.h"
#include "wl_pointer.h"

#include <mir/scene/surface.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mir
{
namespace frontend
{
class RelativePointerManagerV1 : public wayland::RelativePointerManagerV1
{
public:
    RelativePointerManagerV1(wl_resource* resource, std::shared_ptr<shell::Shell> shell);

    class Global : public wayland::RelativePointerManagerV1::Global
    {
    public:
        Global(wl_display* display, std::shared_ptr<shell::Shell> shell);

    private:
        void bind(wl_resource* new_zwp_relative_pointer_manager_v1) override;
        std::shared_ptr<shell::Shell> const shell;
    };

private:
    std::shared_ptr<shell::Shell> const shell;

    void get_relative_pointer(wl_resource* id, wl_resource* pointer) override;
};

class RelativePointerV1 : public wayland::RelativePointerV1
{
public:
    RelativePointerV1(wl_resource* id, WlPointer* pointer);

private:
    wayland::Weak<WlPointer> const pointer;
};
}
}

auto mir::frontend::create_relative_pointer_unstable_v1(wl_display *display, std::shared_ptr<shell::Shell> shell)
    -> std::shared_ptr<void>
{
    return std::make_shared<RelativePointerManagerV1::Global>(display, std::move(shell));
}

mir::frontend::RelativePointerManagerV1::Global::Global(wl_display* display, std::shared_ptr<shell::Shell> shell) :
    wayland::RelativePointerManagerV1::Global::Global{display, Version<1>{}},
    shell{std::move(shell)}
{
}

void mir::frontend::RelativePointerManagerV1::Global::bind(wl_resource* new_zwp_relative_pointer_manager_v1)
{
    new RelativePointerManagerV1{new_zwp_relative_pointer_manager_v1, shell};
}

mir::frontend::RelativePointerManagerV1::RelativePointerManagerV1(wl_resource* resource, std::shared_ptr<shell::Shell> shell) :
    wayland::RelativePointerManagerV1{resource, Version<1>{}},
    shell{std::move(shell)}
{
}

void mir::frontend::RelativePointerManagerV1::get_relative_pointer(wl_resource* id, wl_resource* pointer)
{
    new RelativePointerV1{id, dynamic_cast<WlPointer*>(wayland::Pointer::from(pointer))};
}

mir::frontend::RelativePointerV1::RelativePointerV1(wl_resource* id, WlPointer* pointer) :
    wayland::RelativePointerV1{id, Version<1>{}},
    pointer{pointer}
{
    pointer->set_relative_pointer(this);
}
