/*
 * Copyright © Canonical Ltd.
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

#include "zwp_relative_pointer_v1.h"
#include "wl_pointer.h"

#include <mir/scene/surface.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mwrs = mir::wayland;

namespace mir
{
namespace frontend
{
class RelativePointerManagerV1 : public wayland::RelativePointerManagerV1
{
public:
    RelativePointerManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::RelativePointerManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<shell::Shell> shell);

private:
    std::shared_ptr<shell::Shell> const shell;

    using wayland::RelativePointerManagerV1::get_relative_pointer;
    auto get_relative_pointer(
        wayland::Weak<wayland::Pointer> const& pointer,
        rust::Box<wayland::RelativePointerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::RelativePointerV1> override;
};

class RelativePointerV1 : public wayland::RelativePointerV1
{
public:
    RelativePointerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::RelativePointerV1Middleware> instance,
        uint32_t object_id,
        WlPointer* pointer);

private:
    wayland::Weak<WlPointer> const pointer;
};
}
}

auto mir::frontend::create_relative_pointer_unstable_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::RelativePointerManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<shell::Shell> shell)
-> std::shared_ptr<wayland::RelativePointerManagerV1>
{
    return std::make_shared<RelativePointerManagerV1>(
        std::move(client), std::move(instance), object_id, std::move(shell));
}

mir::frontend::RelativePointerManagerV1::RelativePointerManagerV1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::RelativePointerManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<shell::Shell> shell) :
    wayland::RelativePointerManagerV1{std::move(client), std::move(instance), object_id},
    shell{std::move(shell)}
{
}

auto mir::frontend::RelativePointerManagerV1::get_relative_pointer(
    wayland::Weak<wayland::Pointer> const& pointer,
    rust::Box<wayland::RelativePointerV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<wayland::RelativePointerV1>
{
    return std::make_shared<RelativePointerV1>(
        client, std::move(child_instance), child_object_id,
        wayland::Pointer::from<WlPointer>(pointer));
}

mir::frontend::RelativePointerV1::RelativePointerV1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::RelativePointerV1Middleware> instance,
    uint32_t object_id,
    WlPointer* pointer) :
    wayland::RelativePointerV1{std::move(client), std::move(instance), object_id},
    pointer{mwrs::make_weak(pointer)}
{
    if (pointer)
        pointer->set_relative_pointer(this);
}
