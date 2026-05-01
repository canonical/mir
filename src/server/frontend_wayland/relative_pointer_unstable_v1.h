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

#ifndef MIR_FRONTEND_RELATIVE_POINTER_UNSTABLE_V1_H
#define MIR_FRONTEND_RELATIVE_POINTER_UNSTABLE_V1_H

#include "wayland_rs/wayland_rs_cpp/include/relative_pointer_unstable_v1.h"

#include <memory>

namespace mir
{
namespace shell { class Shell; }

namespace frontend
{
class RelativePointerManagerV1 : public wayland_rs::ZwpRelativePointerManagerV1Impl
{
public:
    explicit RelativePointerManagerV1(std::shared_ptr<shell::Shell> shell);
    auto get_relative_pointer(wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer)
        -> std::shared_ptr<wayland_rs::ZwpRelativePointerV1Impl> override;

private:
    std::shared_ptr<shell::Shell> const shell;
};
}
}

#endif  // MIR_FRONTEND_RELATIVE_POINTER_UNSTABLE_V1_H
