/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_

#include "mir/graphics/drm_formats.h"
#include "mir/graphics/platform.h"
#include "mir/test/doubles/null_display_sink.h"
#include "mir/geometry/rectangle.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir_toolkit/common.h"
#include <drm_fourcc.h>

namespace mir
{
namespace test
{
namespace doubles
{

class DummyCPUAddressableDisplayAllocator : public graphics::CPUAddressableDisplayAllocator
{
    class MappableFB : public graphics::CPUAddressableDisplayAllocator::MappableFB
    {
    public:
        MappableFB(geometry::Size size, graphics::DRMFormat format)
            : buffer{std::make_unique<StubBuffer>(size, format.as_mir_format().value())}
        {
        }

        auto map_writeable() -> std::unique_ptr<renderer::software::Mapping<std::byte>> override
        {
            return buffer->map_writeable();
        }

        auto format() const -> MirPixelFormat override
        {
            return buffer->format();
        }

        auto stride() const -> geometry::Stride override
        {
            return buffer->stride();
        }
        auto size() const -> geometry::Size override
        {
            return buffer->size();
        }
    private:
        std::unique_ptr<StubBuffer> const buffer;
    };

public:
    DummyCPUAddressableDisplayAllocator(geometry::Size size)
        : size{size}
    {
    }

    auto supported_formats() const -> std::vector<graphics::DRMFormat> override
    {
        return {graphics::DRMFormat{DRM_FORMAT_ARGB8888}};
    }
    auto alloc_fb(graphics::DRMFormat format) -> std::unique_ptr<graphics::CPUAddressableDisplayAllocator::MappableFB> override
    {
        return std::make_unique<MappableFB>(size, format);
    }
    auto output_size() const -> geometry::Size override
    {
        return size;
    }

private:
    geometry::Size const size;
};

class StubDisplaySink : public NullDisplaySink
{
public:
    StubDisplaySink(geometry::Rectangle const& view_area_)
        : view_area_(view_area_),
          allocator{view_area_.size}
    {
    }

    StubDisplaySink(StubDisplaySink const& s)
        : StubDisplaySink(s.view_area_)
    {
    }

    geometry::Rectangle view_area() const override { return view_area_; }

private:
    auto maybe_create_allocator(graphics::DisplayAllocator::Tag const& tag)
        -> graphics::DisplayAllocator* override
    {
        if (dynamic_cast<graphics::CPUAddressableDisplayAllocator::Tag const*>(&tag))
        {
            return &allocator;
        }
        return nullptr;
    }

    geometry::Rectangle view_area_;
    DummyCPUAddressableDisplayAllocator allocator;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_ */
