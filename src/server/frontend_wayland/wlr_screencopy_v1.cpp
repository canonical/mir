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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "wlr_screencopy_v1.h"

#include "mir/compositor/screen_shooter.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/buffer.h"
#include "mir/executor.h"
#include "wayland_wrapper.h"
#include "wayland_timespec.h"
#include "output_manager.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{
struct WlrScreencopyV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    OutputManager& output_manager;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<compositor::ScreenShooter> const screen_shooter;

    auto extents_of_output(wl_resource* output) -> geom::Rectangle;
};

class WlrScreencopyManagerV1Global
    : public wayland::WlrScreencopyManagerV1::Global
{
public:
    WlrScreencopyManagerV1Global(wl_display* display, std::shared_ptr<WlrScreencopyV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
};

class WlrScreencopyManagerV1
    : public wayland::WlrScreencopyManagerV1
{
public:
    WlrScreencopyManagerV1(wl_resource* resource, std::shared_ptr<WlrScreencopyV1Ctx> const& ctx);

private:
    void capture_output(struct wl_resource* frame, int32_t overlay_cursor, struct wl_resource* output) override;
    void capture_output_region(
        struct wl_resource* frame,
        int32_t overlay_cursor,
        struct wl_resource* output,
        int32_t x, int32_t y,
        int32_t width, int32_t height) override;

    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
};

class WlrScreencopyFrameV1
    : public wayland::WlrScreencopyFrameV1
{
public:
    WlrScreencopyFrameV1(
        wl_resource* resource,
        std::shared_ptr<WlrScreencopyV1Ctx> const& ctx,
        geometry::Rectangle const& area);

private:
    /// From wayland::WlrScreencopyFrameV1
    /// @{
    void copy(struct wl_resource* buffer) override;
    void copy_with_damage(struct wl_resource* buffer) override;
    /// @}

    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
    geometry::Rectangle const area;
    geometry::Stride const stride;
};
}
}

auto mf::create_wlr_screencopy_manager_unstable_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    OutputManager& output_manager,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter)
-> std::shared_ptr<wayland::WlrScreencopyManagerV1::Global>
{
    auto ctx = std::shared_ptr<WlrScreencopyV1Ctx>{new WlrScreencopyV1Ctx{
        wayland_executor,
        output_manager,
        allocator,
        screen_shooter}};
    return std::make_shared<WlrScreencopyManagerV1Global>(display, std::move(ctx));
}

auto mf::WlrScreencopyV1Ctx::extents_of_output(wl_resource* output) -> geom::Rectangle
{
    geom::Rectangle extents;
    if (!output_manager.with_config_for(output, [&](mg::DisplayConfigurationOutput const& config)
        {
            extents = config.extents();
        }))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "No output for wl_output@" + std::to_string(wl_resource_get_id(output))));
    }
    return extents;
}

mf::WlrScreencopyManagerV1Global::WlrScreencopyManagerV1Global(
    wl_display* display,
    std::shared_ptr<WlrScreencopyV1Ctx> const& ctx)
    : Global{display, Version<3>()},
      ctx{ctx}
{
}

void mf::WlrScreencopyManagerV1Global::bind(wl_resource* new_resource)
{
    new WlrScreencopyManagerV1{new_resource, ctx};
}

mf::WlrScreencopyManagerV1::WlrScreencopyManagerV1(
    wl_resource* resource,
    std::shared_ptr<WlrScreencopyV1Ctx> const& ctx)
    : wayland::WlrScreencopyManagerV1{resource, Version<3>()},
      ctx{ctx}
{
}

void mf::WlrScreencopyManagerV1::capture_output(
    struct wl_resource* frame,
    int32_t overlay_cursor,
    struct wl_resource* output)
{
    (void)overlay_cursor;
    new WlrScreencopyFrameV1{frame, ctx, ctx->extents_of_output(output)};
}

void mf::WlrScreencopyManagerV1::capture_output_region(
    struct wl_resource* frame,
    int32_t overlay_cursor,
    struct wl_resource* output,
    int32_t x, int32_t y,
    int32_t width, int32_t height)
{
    (void)overlay_cursor;
    auto const intersection = geom::Rectangle{{x, y}, {width, height}}
        .intersection_with(ctx->extents_of_output(output));
    new WlrScreencopyFrameV1{frame, ctx, intersection};
}

mf::WlrScreencopyFrameV1::WlrScreencopyFrameV1(
    wl_resource* resource,
    std::shared_ptr<WlrScreencopyV1Ctx> const& ctx,
    geometry::Rectangle const& area)
    : wayland::WlrScreencopyFrameV1{resource, Version<3>()},
      ctx{ctx},
      area{area},
      stride{area.size.width.as_uint32_t() * 4}
{
    send_buffer_event(
        wayland::Shm::Format::argb8888,
        area.size.width.as_uint32_t(),
        area.size.height.as_uint32_t(),
        stride.as_uint32_t());
    if (version_supports_buffer_done())
    {
        send_buffer_done_event();
    }
}

void mf::WlrScreencopyFrameV1::copy(struct wl_resource* buffer)
{
    auto const graphics_buffer = ctx->allocator->buffer_from_shm(buffer, ctx->wayland_executor, [](){});
    if (graphics_buffer->pixel_format() != mir_pixel_format_argb_8888)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid pixel format %d",
            graphics_buffer->pixel_format()));
    }
    if (graphics_buffer->size() != area.size)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid buffer size %dx%d, should be %dx%d",
            graphics_buffer->size().width.as_int(),
            graphics_buffer->size().height.as_int(),
            area.size.width.as_int(),
            area.size.height.as_int()));
    }
    auto const buffer_stride = geom::Stride{wl_shm_buffer_get_stride(wl_shm_buffer_get(buffer))};
    if (buffer_stride != stride)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid stride %d, should be %d",
            buffer_stride.as_int(),
            stride.as_int()));
    }
    auto const target = std::dynamic_pointer_cast<renderer::software::WriteMappableBuffer>(graphics_buffer);
    if (!target)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Failed to get write-mappable buffer out of Wayland SHM buffer"));
    }
    ctx->screen_shooter->capture(std::move(target), area,
        [wayland_executor=ctx->wayland_executor, self=mw::make_weak(this)](std::optional<time::Timestamp> captured_time)
        {
            wayland_executor->spawn([self, captured_time]()
                {
                    if (self)
                    {
                        if (captured_time)
                        {
                            self.value().send_flags_event(Flags::y_invert);

                            WaylandTimespec const timespec{captured_time.value()};
                            self.value().send_ready_event(
                                timespec.tv_sec_hi,
                                timespec.tv_sec_lo,
                                timespec.tv_nsec);
                        }
                        else
                        {
                            self.value().send_failed_event();
                        }
                    }
                });
        });
}

void mf::WlrScreencopyFrameV1::copy_with_damage(struct wl_resource* buffer)
{
    // TODO
    copy(buffer);
}
