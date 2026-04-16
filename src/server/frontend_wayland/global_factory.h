/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GLOBAL_FACTORY_H
#define MIR_GLOBAL_FACTORY_H

#include "wayland_rs/wayland_rs_cpp/include/global_factory.h"

namespace mir
{
namespace wayland
{
class GlobalFactory : public wayland_rs::GlobalFactory {
public:
    GlobalFactory();

    auto create_ext_data_control_manager_v1() -> std::unique_ptr<wayland_rs::ExtDataControlManagerV1Impl> override;
};
}
}

#endif //MIR_GLOBAL_FACTORY_H
