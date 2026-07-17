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

#ifndef MIRAL_DECORATION_BUFFERS_H_
#define MIRAL_DECORATION_BUFFERS_H_

#include <mir/geometry/size.h>
#include <miral/decoration_surface.h>
#include <mir_toolkit/common.h>

#include <cstdint>
#include <memory>

namespace mir::graphics
{
class Buffer;
class GraphicBufferAllocator;
}

namespace miral
{

/// Software buffer allocation for custom decoration rendering.
/// Constructed by MirAL when calling render methods; compositor authors
/// receive a reference and do not construct this type themselves.
/// \remark Since MirAL 6.0
class DecorationBuffers
{
public:
    /// Writable view of a software decoration buffer.
    /// \remark Since MirAL 6.0
    class Mapping
    {
    public:
        ~Mapping();

        Mapping(Mapping const&) = delete;
        auto operator=(Mapping const&) -> Mapping& = delete;
        Mapping(Mapping&&) noexcept;
        auto operator=(Mapping&&) noexcept -> Mapping&;

        auto data() -> uint8_t*;
        auto stride() const -> mir::geometry::Stride;
        auto size() const -> mir::geometry::Size;
        auto format() const -> MirPixelFormat;
        auto pixels32() -> uint32_t*;

    private:
        struct Self;
        std::unique_ptr<Self> self;

        friend class DecorationBuffers;
        explicit Mapping(std::shared_ptr<mir::graphics::Buffer> buffer);
    };

    /// Software buffer and its writable mapping, allocated together.
    /// \remark Since MirAL 6.0
    struct Pixels
    {
        DecorationSurface surface;
        Mapping mapping;
    };

    DecorationBuffers(DecorationBuffers const&) = delete;
    auto operator=(DecorationBuffers const&) -> DecorationBuffers& = delete;
    ~DecorationBuffers();

    /// Allocate a software buffer of the given pixel dimensions and return it with a mapping.
    auto create_mapping(mir::geometry::Size size, MirPixelFormat format) -> Pixels;

private:
    struct Self;
    std::unique_ptr<Self> self;

    friend class RendererAdapter;
    explicit DecorationBuffers(std::shared_ptr<mir::graphics::GraphicBufferAllocator> allocator);
};

}

#endif // MIRAL_DECORATION_BUFFERS_H_
