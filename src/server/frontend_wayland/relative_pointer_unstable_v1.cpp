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

#include "relative_pointer_unstable_v1.h"
#include "wl_pointer.h"

#include <mir/scene/surface.h>
#include <mir/shell/shell.h>
#include <mir/shell/surface_specification.h>

namespace mw = mir::wayland_rs;

namespace mir
{
namespace frontend
{
class RelativePointerV1 : public mw::ZwpRelativePointerV1Impl, public std::enable_shared_from_this<RelativePointerV1>
{
public:
    RelativePointerV1(wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer);

private:
    mw::Weak<WlPointer> const pointer_;
};
}
}

mir::frontend::RelativePointerManagerV1::RelativePointerManagerV1(std::shared_ptr<shell::Shell> shell) :
    shell{std::move(shell)}
{
}

auto mir::frontend::RelativePointerManagerV1::get_relative_pointer(wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer) -> std::shared_ptr<wayland_rs::ZwpRelativePointerV1Impl>
{
    return std::make_shared<RelativePointerV1>(pointer);
}

mir::frontend::RelativePointerV1::RelativePointerV1(wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer) :
    pointer_{WlPointer::from(&pointer.value())->shared_from_this()}
{
    pointer_.value().set_relative_pointer(shared_from_this());
}
