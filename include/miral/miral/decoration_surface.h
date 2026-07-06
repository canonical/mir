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

#ifndef MIRAL_DECORATION_SURFACE_H_
#define MIRAL_DECORATION_SURFACE_H_

#include <memory>

namespace mir::graphics { class Buffer; }

namespace miral
{

/// Move-only render output returned from DecorationStrategy::render_* methods.
/// Obtain this from DecorationBuffers::create_mapping(); paint via the accompanying
/// Mapping, then return std::move(pixels.surface) to submit the buffer to MirAL.
/// \remark Since MirAL 6.0
class DecorationSurface
{
public:
    ~DecorationSurface();

    DecorationSurface(DecorationSurface&&) noexcept;
    auto operator=(DecorationSurface&&) noexcept -> DecorationSurface&;

    DecorationSurface(DecorationSurface const&) = delete;
    auto operator=(DecorationSurface const&) -> DecorationSurface& = delete;

private:
    struct Self;
    std::unique_ptr<Self> self;

    explicit DecorationSurface(std::shared_ptr<mir::graphics::Buffer> buffer);

    auto take_buffer() -> std::shared_ptr<mir::graphics::Buffer>;

    friend class DecorationBuffers;
    friend class RendererAdapter;
};

}

#endif // MIRAL_DECORATION_SURFACE_H_