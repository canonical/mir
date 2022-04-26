/*
 * Copyright © 2022 Canonical Ltd.
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

#ifndef MIR_FRONTEND_WLR_SCREENCOPY_V1_H
#define MIR_FRONTEND_WLR_SCREENCOPY_V1_H

#include "wlr-screencopy-unstable-v1_wrapper.h"

#include <memory>

namespace mir
{
class Executor;
namespace compositor
{
class ScreenShooter;
}
namespace graphics
{
class GraphicBufferAllocator;
}
namespace frontend
{
class OutputManager;

auto create_wlr_screencopy_manager_unstable_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    OutputManager& output_manager,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter)
-> std::shared_ptr<wayland::WlrScreencopyManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_WLR_SCREENCOPY_V1_H
