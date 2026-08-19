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

#include <mir/compositor/buffer_stream.h>
#include <mir/geometry/dimensions.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>

namespace geomtery = mir::geometry;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;

namespace
{
class BufferPainter
{
public:
    struct HorizontalLine
    {
        geomtery::X start;
        geomtery::X end;
        geomtery::Y y;
        geomtery::Height thickness;
    };

    struct VerticalLine
    {
        geomtery::X x;
        geomtery::Y start;
        geomtery::Y end;
        geomtery::Width thickness;
    };

    explicit BufferPainter(mg::Buffer& buffer) :
        writable{mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}))},
        mapping{writable->map_writeable()},
        stride{mapping->stride()},
        size{buffer.size()}
    { assert(mapping->format() == format); }

    void clear() { std::memset(mapping->data(), 0, mapping->len()); }

    void fill_circle()
    {
        auto const cx = geomtery::as_x((size.width - geomtery::DeltaX{1}) / 2.0f);
        auto const cy = geomtery::as_y((size.height - geomtery::DeltaY{1}) / 2.0f);
        auto const r = std::min(cx.as_value(), cy.as_value());
        auto const r_sq = r * r;

        for (geomtery::Y y{0}; y < geomtery::as_y(size.height); y = y + geomtery::DeltaY{1})
        {
            for (geomtery::X x{0}; x < geomtery::as_x(size.width); x = x + geomtery::DeltaX{1})
            {
                auto const dx = x - cx;
                auto const dy = y - cy;
                auto const delta = dx.as_value() * dx.as_value() + dy.as_value() * dy.as_value();

                if (delta <= r_sq)
                    set_pixel(geomtery::Point{x, y}, disc_grey);
            }
        }
    }

    void draw_horizontal_line(HorizontalLine const& line)
    {
        auto const true_start = std::min<geomtery::X>(line.start, line.end);
        auto const true_end = std::max<geomtery::X>(line.start, line.end);
        for (int t = 0; t < line.thickness.as_value(); ++t)
            for (auto x = true_start; x <= true_end; x = x + geomtery::DeltaX{1})
                set_pixel(geomtery::Point{x, line.y + geomtery::DeltaY{t}}, icon_grey);
    }

    void draw_vertical_line(VerticalLine const& line)
    {
        auto const true_start = std::min<geomtery::Y>(line.start, line.end);
        auto const true_end = std::max<geomtery::Y>(line.start, line.end);
        for (int t = 0; t < line.thickness.as_value(); ++t)
            for (auto y = true_start; y <= true_end; y = y + geomtery::DeltaY{1})
                set_pixel(geomtery::Point{line.x + geomtery::DeltaX{t}, y}, icon_grey);
    }

    auto buffer_size() const -> geomtery::Size { return size; }

private:
    void set_pixel(geomtery::Point const& point, uint8_t grey)
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
    inline uint8_t static disc_grey = 40;
    inline uint8_t static icon_grey = 210;
    inline uint8_t static background_alpha = 150;

    static auto constexpr format = mir_pixel_format_argb_8888;
    std::shared_ptr<mrs::WriteMappable> const writable;
    std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
    geomtery::Stride const stride;
    geomtery::Size const size;
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
    auto const center_x = geomtery::as_x((w - geomtery::DeltaX{1}) / 2);
    auto const center_y = geomtery::as_y((h - geomtery::DeltaY{1}) / 2);
    auto const tip_n = center_y - geomtery::DeltaY{tip_dist};
    auto const tip_s = center_y + geomtery::DeltaY{tip_dist};
    auto const tip_w = center_x - geomtery::DeltaX{tip_dist};
    auto const tip_e = center_x + geomtery::DeltaX{tip_dist};
    auto const base_n = center_y - geomtery::DeltaY{base_dist};
    auto const base_s = center_y + geomtery::DeltaY{base_dist};
    auto const base_w = center_x - geomtery::DeltaX{base_dist};
    auto const base_e = center_x + geomtery::DeltaX{base_dist};

    // Stems
    painter.draw_vertical_line({
        .x = center_x - geomtery::DeltaX{stem_thickness / 2},
        .start = base_n,
        .end = base_s,
        .thickness = geomtery::Width{stem_thickness},
    });
    painter.draw_horizontal_line({
        .start = base_w,
        .end = base_e,
        .y = center_y - geomtery::DeltaY{stem_thickness / 2},
        .thickness = geomtery::Height{stem_thickness},
    });

    // North arrowhead — rows from tip_n+1 to base_n, width grows by 1 px per row
    for (auto y = tip_n + geomtery::DeltaY{1}; y <= base_n; y = y + geomtery::DeltaY{1})
        painter.draw_horizontal_line({
            .start = center_x - geomtery::DeltaX((y - tip_n).as_value()),
            .end = center_x + geomtery::DeltaX((y - tip_n).as_value()),
            .y = y,
            .thickness = geomtery::Height{1},
        });

    // South arrowhead
    for (auto y = base_s; y < tip_s; y = y + geomtery::DeltaY{1})
        painter.draw_horizontal_line({
            .start = center_x - geomtery::DeltaX((tip_s - y).as_value()),
            .end = center_x + geomtery::DeltaX((tip_s - y).as_value()),
            .y = y,
            .thickness = geomtery::Height{1},
        });

    // West arrowhead — columns from tip_w+1 to base_w
    for (auto x = tip_w + geomtery::DeltaX{1}; x <= base_w; x = x + geomtery::DeltaX{1})
        painter.draw_vertical_line({
            .x = x,
            .start = center_y - geomtery::DeltaY((x - tip_w).as_value()),
            .end = center_y + geomtery::DeltaY((x - tip_w).as_value()),
            .thickness = geomtery::Width{1},
        });

    // East arrowhead
    for (auto x = base_e; x < tip_e; x = x + geomtery::DeltaX{1})
        painter.draw_vertical_line({
            .x = x,
            .start = center_y - geomtery::DeltaY((tip_e - x).as_value()),
            .end = center_y + geomtery::DeltaY((tip_e - x).as_value()),
            .thickness = geomtery::Width{1},
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
    auto const corner_x = geomtery::as_x(w / 4);
    auto const corner_y = geomtery::as_y(h / 4);

    painter.draw_horizontal_line({
        .start = corner_x,
        .end = corner_x + geomtery::DeltaX{arm_len},
        .y = corner_y,
        .thickness = geomtery::Height{thickness},
    });
    painter.draw_vertical_line({
        .x = corner_x,
        .start = corner_y,
        .end = corner_y + geomtery::DeltaY{arm_len},
        .thickness = geomtery::Width{thickness},
    });
}

static void fill_zoom_buffer(mg::Buffer& buffer, bool zoom_in)
{
    BufferPainter painter{buffer};
    painter.clear();
    painter.fill_circle();

    // Draw + (zoom in) or – (zoom out) symbol.
    auto const [w, h] = painter.buffer_size();
    int const bar_thickness = std::max(geomtery::Width{2}, w / 12).as_value();
    int const bar_len = (w * 5 / 8).as_value();

    painter.draw_horizontal_line({
        .start = geomtery::as_x((w - geomtery::Width{bar_len}) / 2),
        .end = geomtery::as_x((w + geomtery::Width{bar_len}) / 2 - geomtery::DeltaX{1}),
        .y = geomtery::as_y((h - geomtery::Height{bar_thickness}) / 2),
        .thickness = geomtery::Height{bar_thickness},
    });

    if (zoom_in)
        painter.draw_vertical_line({
            .x = geomtery::as_x((w - geomtery::Width{bar_thickness}) / 2),
            .start = geomtery::as_y((h - geomtery::Height{bar_len}) / 2),
            .end = geomtery::as_y((h + geomtery::Height{bar_len}) / 2 - geomtery::DeltaY{1}),
            .thickness = geomtery::Width{bar_thickness},
        });
}

static std::string name_for_kind(miral::HandleKind kind)
{
    switch (kind)
    {
    case miral::HandleKind::drag:
        return "magnifier-drag-handle";
    case miral::HandleKind::resize:
        return "magnifier-resize-handle";
    case miral::HandleKind::zoom_in:
        return "magnifier-zoom-in-handle";
    case miral::HandleKind::zoom_out:
        return "magnifier-zoom-out-handle";
    }

    std::unreachable();
}
}

miral::HandleIndicator::HandleIndicator(
    mir::geometry::Rectangle const& initial_rect,
    HandleKind kind,
    mir::compositor::CompositorID capture_compositor_id,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mir::scene::SceneReport> const& scene_report,
    std::shared_ptr<mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>> const&
        display_config_registrar) :
    BasicSurface{
        name_for_kind(kind),
        initial_rect,
        mir_pointer_unconfined,
        miral::create_stream_info(),
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
    case HandleKind::drag:
        fill_drag_buffer(*buffer);
        break;
    case HandleKind::resize:
        fill_resize_buffer(*buffer);
        break;
    case HandleKind::zoom_in:
    case HandleKind::zoom_out:
        fill_zoom_buffer(*buffer, kind == HandleKind::zoom_in);
        break;
    }
    auto const sz = window_size();
    get_streams().begin()->stream->submit_buffer(buffer, sz, geomtery::RectangleD{{0, 0}, sz});

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
