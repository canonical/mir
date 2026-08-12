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

#include "magnifier_handle_indicator.h"
#include "mir/compositor/stream.h"

#include <mir/compositor/buffer_stream.h>
#include <mir/fatal.h>
#include <mir/geometry/dimensions.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace controls = miral::magnifier_controls;
namespace ms = mir::scene;

namespace
{
class BufferPainter
{
public:
    struct HorizontalLine
    {
        geom::X start;
        geom::X end;
        geom::Y y;
        geom::Height thickness;
    };

    struct VerticalLine
    {
        geom::X x;
        geom::Y start;
        geom::Y end;
        geom::Width thickness;
    };

    explicit BufferPainter(mg::Buffer& buffer) :
        writable{mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}))},
        mapping{writable->map_writeable()},
        stride{mapping->stride()},
        size{buffer.size()}
    {
        if (mapping->format() != format)
            MIR_FATAL_ERROR("HandleIndicator buffer has an unexpected pixel format");
    }

    void clear() { std::memset(mapping->data(), 0, mapping->len()); }

    void fill_circle()
    {
        auto const cx = geom::as_x((size.width - geom::DeltaX{1}) / 2.0f);
        auto const cy = geom::as_y((size.height - geom::DeltaY{1}) / 2.0f);
        auto const r = std::min(cx.as_value(), cy.as_value());
        auto const r_sq = r * r;

        for (geom::Y y{0}; y < geom::as_y(size.height); y = y + geom::DeltaY{1})
        {
            for (geom::X x{0}; x < geom::as_x(size.width); x = x + geom::DeltaX{1})
            {
                auto const dx = x - cx;
                auto const dy = y - cy;
                auto const delta = dx.as_value() * dx.as_value() + dy.as_value() * dy.as_value();

                if (delta <= r_sq)
                    set_pixel(geom::Point{x, y}, disc_grey);
            }
        }
    }

    void draw_horizontal_line(HorizontalLine const& line)
    {
        auto const true_start = std::min<geom::X>(line.start, line.end);
        auto const true_end = std::max<geom::X>(line.start, line.end);
        for (int t = 0; t < line.thickness.as_value(); ++t)
            for (auto x = true_start; x <= true_end; x = x + geom::DeltaX{1})
                set_pixel(geom::Point{x, line.y + geom::DeltaY{t}}, icon_grey);
    }

    void draw_vertical_line(VerticalLine const& line)
    {
        auto const true_start = std::min<geom::Y>(line.start, line.end);
        auto const true_end = std::max<geom::Y>(line.start, line.end);
        for (int t = 0; t < line.thickness.as_value(); ++t)
            for (auto y = true_start; y <= true_end; y = y + geom::DeltaY{1})
                set_pixel(geom::Point{line.x + geom::DeltaX{t}, y}, icon_grey);
    }

    auto buffer_size() const -> geom::Size { return size; }

private:
    void set_pixel(geom::Point const& point, uint8_t grey)
    {
        auto const x = point.x.as_value();
        auto const y = point.y.as_value();
        if (x < 0 || x >= size.width.as_value() || y < 0 || y >= size.height.as_value())
            return;

        // For mir_pixel_format_argb_8888 on little-endian: memory layout [B, G, R, A].
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        std::span<std::uint8_t> const pixels{reinterpret_cast<std::uint8_t*>(mapping->data()), mapping->len()};
        auto const offset = y * stride.as_value() + x * MIR_BYTES_PER_PIXEL(format);
        auto const pixel = pixels.subspan(offset, MIR_BYTES_PER_PIXEL(format));
        pixel[0] = pixel[1] = pixel[2] = grey;
        pixel[3] = background_alpha;
    }

    /// Painter colour/alpha constants for handle indicator graphics.
    inline uint8_t static disc_grey = 24;
    inline uint8_t static icon_grey = 124;
    inline uint8_t static background_alpha = 150;

    static auto constexpr format = mir_pixel_format_argb_8888;
    std::shared_ptr<mrs::WriteMappable> const writable;
    std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
    geom::Stride const stride;
    geom::Size const size;
};

static void fill_drag_buffer(mg::Buffer& buffer)
{
    BufferPainter painter{buffer};
    painter.clear();
    painter.fill_circle();

    // Draw a drag-pan icon: four arrowheads (N/S/E/W) connected by slim stems.
    static int constexpr tip_dist = 15;
    static int constexpr head_len = 5;
    static int constexpr stem_thickness = 3;
    static int constexpr base_dist = (tip_dist - head_len);

    auto const [w, h] = painter.buffer_size();
    auto const center_x = geom::as_x((w - geom::DeltaX{1}) / 2);
    auto const center_y = geom::as_y((h - geom::DeltaY{1}) / 2);
    auto const tip_n = center_y - geom::DeltaY{tip_dist};
    auto const tip_s = center_y + geom::DeltaY{tip_dist};
    auto const tip_w = center_x - geom::DeltaX{tip_dist};
    auto const tip_e = center_x + geom::DeltaX{tip_dist};
    auto const base_n = center_y - geom::DeltaY{base_dist};
    auto const base_s = center_y + geom::DeltaY{base_dist};
    auto const base_w = center_x - geom::DeltaX{base_dist};
    auto const base_e = center_x + geom::DeltaX{base_dist};

    // Stems
    painter.draw_vertical_line({
        .x = center_x - geom::DeltaX{stem_thickness / 2},
        .start = base_n,
        .end = base_s,
        .thickness = geom::Width{stem_thickness},
    });
    painter.draw_horizontal_line({
        .start = base_w,
        .end = base_e,
        .y = center_y - geom::DeltaY{stem_thickness / 2},
        .thickness = geom::Height{stem_thickness},
    });

    // North arrowhead — rows from tip_n+1 to base_n, width grows by 1 px per row
    for (auto y = tip_n + geom::DeltaY{1}; y <= base_n; y = y + geom::DeltaY{1})
        painter.draw_horizontal_line({
            .start = center_x - geom::DeltaX((y - tip_n).as_value()),
            .end = center_x + geom::DeltaX((y - tip_n).as_value()),
            .y = y,
            .thickness = geom::Height{1},
        });

    // South arrowhead
    for (auto y = base_s; y < tip_s; y = y + geom::DeltaY{1})
        painter.draw_horizontal_line({
            .start = center_x - geom::DeltaX((tip_s - y).as_value()),
            .end = center_x + geom::DeltaX((tip_s - y).as_value()),
            .y = y,
            .thickness = geom::Height{1},
        });

    // West arrowhead — columns from tip_w+1 to base_w
    for (auto x = tip_w + geom::DeltaX{1}; x <= base_w; x = x + geom::DeltaX{1})
        painter.draw_vertical_line({
            .x = x,
            .start = center_y - geom::DeltaY((x - tip_w).as_value()),
            .end = center_y + geom::DeltaY((x - tip_w).as_value()),
            .thickness = geom::Width{1},
        });

    // East arrowhead
    for (auto x = base_e; x < tip_e; x = x + geom::DeltaX{1})
        painter.draw_vertical_line({
            .x = x,
            .start = center_y - geom::DeltaY((tip_e - x).as_value()),
            .end = center_y + geom::DeltaY((tip_e - x).as_value()),
            .thickness = geom::Width{1},
        });
}

static void fill_resize_buffer(mg::Buffer& buffer)
{
    BufferPainter painter{buffer};
    painter.clear();
    painter.fill_circle();

    // Corner bracket: two 2-px arms meeting at the top-left corner, which is
    // where the resize handle always sits within the magnifier surface.
    static int constexpr arm_len = 12;
    static int constexpr thickness = 2;
    auto const [w, h] = painter.buffer_size();
    auto const corner_x = geom::as_x(w / 4);
    auto const corner_y = geom::as_y(h / 4);

    painter.draw_horizontal_line({
        .start = corner_x,
        .end = corner_x + geom::DeltaX{arm_len},
        .y = corner_y,
        .thickness = geom::Height{thickness},
    });
    painter.draw_vertical_line({
        .x = corner_x,
        .start = corner_y,
        .end = corner_y + geom::DeltaY{arm_len},
        .thickness = geom::Width{thickness},
    });
}

static void fill_zoom_buffer(mg::Buffer& buffer, bool zoom_in)
{
    BufferPainter painter{buffer};
    painter.clear();
    painter.fill_circle();

    // Draw + (zoom in) or – (zoom out) symbol.
    auto const [w, h] = painter.buffer_size();
    int const bar_thickness = std::max(geom::Width{2}, w / 12).as_value();
    int const bar_len = (w * 5 / 8).as_value();

    painter.draw_horizontal_line({
        .start = geom::as_x((w - geom::Width{bar_len}) / 2),
        .end = geom::as_x((w + geom::Width{bar_len}) / 2 - geom::DeltaX{1}),
        .y = geom::as_y((h - geom::Height{bar_thickness}) / 2),
        .thickness = geom::Height{bar_thickness},
    });

    if (zoom_in)
        painter.draw_vertical_line({
            .x = geom::as_x((w - geom::Width{bar_thickness}) / 2),
            .start = geom::as_y((h - geom::Height{bar_len}) / 2),
            .end = geom::as_y((h + geom::Height{bar_len}) / 2 - geom::DeltaY{1}),
            .thickness = geom::Width{bar_thickness},
        });
}

static std::string name_for_kind(controls::HandleKind kind)
{
    switch (kind)
    {
    case controls::HandleKind::drag:
        return "magnifier-drag-handle";
    case controls::HandleKind::resize:
        return "magnifier-resize-handle";
    case controls::HandleKind::zoom_in:
        return "magnifier-zoom-in-handle";
    case controls::HandleKind::zoom_out:
        return "magnifier-zoom-out-handle";
    }

    std::unreachable();
}

auto create_always_has_submitted_buffer_stream_info() -> std::list<ms::StreamInfo>
{
    /// A mc::Stream that always reports having a submitted buffer so that
    /// BasicSurface::generate_renderables() includes the surface in every frame.
    class AlwaysHasSubmittedBufferStream : public mir::compositor::Stream
    {
    public:
        bool has_submitted_buffer() const override { return true; }
    };

    return {ms::StreamInfo{std::make_shared<AlwaysHasSubmittedBufferStream>(), geom::Displacement{}}};
}
}

miral::HandleIndicator::HandleIndicator(
    mir::geometry::Rectangle const& initial_rect,
    controls::HandleKind kind,
    mir::compositor::CompositorID capture_compositor_id,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mir::scene::SceneReport> const& scene_report,
    std::shared_ptr<mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>> const&
        display_config_registrar) :
    BasicSurface{
        name_for_kind(kind),
        initial_rect,
        mir_pointer_unconfined,
        create_always_has_submitted_buffer_stream_info(),
        nullptr,
        scene_report,
        display_config_registrar},
    pool{
        [allocator, initial_rect]
        { return allocator->alloc_software_buffer(initial_rect.size, mir_pixel_format_argb_8888); },
        1},
    capture_compositor_id{capture_compositor_id}
{
    auto buffer = pool.claim();
    switch (kind)
    {
    case controls::HandleKind::drag:
        fill_drag_buffer(*buffer);
        break;
    case controls::HandleKind::resize:
        fill_resize_buffer(*buffer);
        break;
    case controls::HandleKind::zoom_in:
    case controls::HandleKind::zoom_out:
        fill_zoom_buffer(*buffer, kind == controls::HandleKind::zoom_in);
        break;
    }
    auto const sz = window_size();
    get_streams().begin()->stream->submit_buffer(buffer, sz, geom::RectangleD{{0, 0}, sz});

    set_depth_layer(mir_depth_layer_always_on_top);
    set_focus_mode(mir_focus_mode_disabled);
    hide();
}

auto miral::HandleIndicator::generate_renderables(mir::compositor::CompositorID id) const
    -> mir::graphics::RenderableList
{
    if (id == capture_compositor_id)
        return {};
    return BasicSurface::generate_renderables(id);
}
