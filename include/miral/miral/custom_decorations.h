/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_CUSTOM_DECORATIONS_H
#define MIRAL_CUSTOM_DECORATIONS_H

#include <miral/decoration_buffers.h>
#include <miral/decoration_strategy.h>
#include <miral/decoration_surface.h>

#include <memory>

namespace mir { class Server; }

namespace miral
{

/// Installs a custom server-side decoration renderer for the compositor lifetime.
/// Use together with miral::Decorations so clients negotiate the decoration mode you
/// want (for example prefer_ssd(), prefer_csd(), always_ssd(), or always_csd()); this
/// type only replaces how SSD buffers are painted when server-side decorations are active.
/// The renderer strategy is fixed at MirRunner construction; it cannot be changed at runtime.
/// \remark Since MirAL 6.0
class CustomDecorations
{
public:
    explicit CustomDecorations(std::shared_ptr<DecorationStrategy> strategy);

    void operator()(mir::Server& server) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

}

#endif // MIRAL_CUSTOM_DECORATIONS_H
