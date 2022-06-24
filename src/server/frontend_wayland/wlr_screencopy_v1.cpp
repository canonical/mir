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

#include "wlr_screencopy_v1.h"

#include "mir/compositor/screen_shooter.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/buffer.h"
#include "mir/scene/scene_change_notification.h"
#include "mir/frontend/surface_stack.h"
#include "mir/geometry/rectangles.h"
#include "mir/executor.h"
#include "wayland_wrapper.h"
#include "wayland_timespec.h"
#include "output_manager.h"

#include <boost/throw_exception.hpp>
#include <mutex>
#include <optional>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

class mf::WlrScreencopyV1DamageTracker::Area
{
public:
    explicit Area(FrameParams const& params);
    ~Area();

    /// Adds the given damage (damages everything if null), and captures the frame if needed
    void apply_damage(std::optional<geometry::Rectangle> const& damage);
    /// Adds a frame, and immediately captures it if there is already damage
    void add_frame(Frame* frame);

    FrameParams const params;

private:
    void capture_frame();

    /// The amount of damage since the last frame was captured (should be none unless pending_frame is null)
    DamageAmount damage_amount;
    /// Only has meaning when damage_amount is partial
    geom::Rectangle damage_rect;
    /// The frame that will be captured once this capture area takes damage
    wayland::Weak<Frame> pending_frame;
};

namespace mir
{
namespace frontend
{
struct WlrScreencopyV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<compositor::ScreenShooter> const screen_shooter;
    std::shared_ptr<SurfaceStack> const surface_stack;
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

class WlrScreencopyFrameV1;

class WlrScreencopyManagerV1
    : public wayland::WlrScreencopyManagerV1
{
public:
    WlrScreencopyManagerV1(wl_resource* resource, std::shared_ptr<WlrScreencopyV1Ctx> const& ctx);

    void capture_on_damage(WlrScreencopyV1DamageTracker::Frame* frame);

private:
    /// From wayland::WlrScreencopyManagerV1
    /// @{
    void capture_output(wl_resource* frame, int32_t overlay_cursor, wl_resource* output) override;
    void capture_output_region(
        wl_resource* frame,
        int32_t overlay_cursor,
        wl_resource* output,
        int32_t x, int32_t y,
        int32_t width, int32_t height) override;
    /// @}

    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
    WlrScreencopyV1DamageTracker damage_tracker;
};

class WlrScreencopyFrameV1
    : public wayland::WlrScreencopyFrameV1,
      WlrScreencopyV1DamageTracker::Frame
{
public:
    WlrScreencopyFrameV1(
        wl_resource* resource,
        WlrScreencopyManagerV1* manager,
        std::shared_ptr<WlrScreencopyV1Ctx> const& ctx,
        WlrScreencopyV1DamageTracker::FrameParams const& params);

    /// From WlrScreencopyV1DamageTracker::Frame
    /// @{
    auto destroyed_flag() const -> std::shared_ptr<bool const> override { return LifetimeTracker::destroyed_flag(); }
    auto parameters() const -> WlrScreencopyV1DamageTracker::FrameParams const& override { return params; }
    void capture(std::optional<geometry::Rectangle> const& damage) override;
    /// @}

private:
    void prepare_target(wl_resource* buffer);
    void report_result(std::optional<time::Timestamp> captured_time, std::optional<geom::Rectangle> const& damage);

    /// From wayland::WlrScreencopyFrameV1
    /// @{
    void copy(wl_resource* buffer) override;
    void copy_with_damage(wl_resource* buffer) override;
    /// @}

    wayland::Weak<WlrScreencopyManagerV1> const manager;
    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
    WlrScreencopyV1DamageTracker::FrameParams const params;
    geometry::Stride const stride;

    /// Only accessed from the Wayland thread
    /// @{
    bool copy_has_been_called{false};
    bool should_send_damage{false};
    std::shared_ptr<renderer::software::WriteMappableBuffer> target;
    /// @}
};
}
}

auto mf::create_wlr_screencopy_manager_unstable_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
    std::shared_ptr<SurfaceStack> const& surface_stack)
-> std::shared_ptr<wayland::WlrScreencopyManagerV1::Global>
{
    auto ctx = std::shared_ptr<WlrScreencopyV1Ctx>{new WlrScreencopyV1Ctx{
        wayland_executor,
        allocator,
        screen_shooter,
        surface_stack}};
    return std::make_shared<WlrScreencopyManagerV1Global>(display, std::move(ctx));
}

mf::WlrScreencopyV1DamageTracker::WlrScreencopyV1DamageTracker(Executor& wayland_executor, SurfaceStack& surface_stack)
    : wayland_executor{wayland_executor},
      surface_stack{surface_stack}
{
}

mf::WlrScreencopyV1DamageTracker::~WlrScreencopyV1DamageTracker()
{
    if (change_notifier)
    {
        surface_stack.remove_observer(change_notifier);
    }
}

void mf::WlrScreencopyV1DamageTracker::capture_on_damage(Frame* frame)
{
    if (!change_notifier)
    {
        // We create the change notifier the first time a client requests a frame with damage
        create_change_notifier();
    }
    auto const frame_params = frame->parameters();
    auto const area = std::find_if(
        begin(areas),
        end(areas),
        [&](auto area){ return area.params == frame_params; });
    if (area != end(areas))
    {
        area->add_frame(frame);
    }
    else
    {
        // We capture the given frame immediately, and also push an empty capture area so that if we get another
        // capture request with the same params it will wait for damage since this frame.
        frame->capture(std::nullopt);
        areas.emplace_back(frame_params);
        // If an unusually high number of capture areas have been created for some reason, clear the list rather than
        // getting bogged down (it's ok, worst case scenario waiting for damage doesn't work)
        if (areas.size() > 100)
        {
            areas.clear();
        }
    }
}

void mf::WlrScreencopyV1DamageTracker::create_change_notifier()
{
    auto callback = [this, weak_self=mw::make_weak(this)](std::optional<geom::Rectangle> const& damage)
        {
            wayland_executor.spawn([weak_self, damage]()
                {
                    if (weak_self)
                    {
                        for (auto& area : weak_self.value().areas)
                        {
                            area.apply_damage(damage);
                        }
                    }
                });
        };
    change_notifier = std::make_shared<ms::SceneChangeNotification>(
        [callback](){ callback(std::nullopt); },
        [callback=std::move(callback)](int, geom::Rectangle const& damage){ callback(damage); });
    surface_stack.add_observer(change_notifier);
}

mf::WlrScreencopyV1DamageTracker::Area::Area(FrameParams const& params)
    : params{params},
      damage_amount{DamageAmount::none}
{
}

mf::WlrScreencopyV1DamageTracker::Area::~Area()
{
    capture_frame();
}

void mf::WlrScreencopyV1DamageTracker::Area::apply_damage(std::optional<geom::Rectangle> const& damage)
{
    if (damage && damage_amount != DamageAmount::full)
    {
        auto const intersection = intersection_of(damage.value(), params.area);
        if (intersection.size != geom::Size{})
        {
            if (damage_amount == DamageAmount::partial)
            {
                damage_rect = geom::Rectangles{damage_rect, intersection}.bounding_rectangle();
            }
            else // damage_amount == DamageAmount::none
            {
                damage_amount = DamageAmount::partial;
                damage_rect = intersection;
            }
        }
    }
    else
    {
        // If damage amount is already full, or if damage is nullopt (and thus full damage should be applied)
        damage_amount = DamageAmount::full;
    }

    if (damage_amount != DamageAmount::none)
    {
        capture_frame();
    }
}

void mf::WlrScreencopyV1DamageTracker::Area::add_frame(Frame* frame)
{
    // Do not allow multiple frames to build up, instead capture now
    capture_frame();
    pending_frame = mw::make_weak(frame);
    if (damage_amount != DamageAmount::none)
    {
        capture_frame();
    }
}

void mf::WlrScreencopyV1DamageTracker::Area::capture_frame()
{
    if (!pending_frame)
    {
        return;
    }

    switch(damage_amount)
    {
    case DamageAmount::none:
    {
        auto const zero_size_damage = geom::Rectangle{params.area.top_left, {}};
        pending_frame.value().capture(zero_size_damage);
    }   break;

    case DamageAmount::partial:
        pending_frame.value().capture(damage_rect);
        break;

    case DamageAmount::full:
        pending_frame.value().capture(std::nullopt);
    }

    damage_amount = DamageAmount::none;
    pending_frame = {};
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
      ctx{ctx},
      damage_tracker{*ctx->wayland_executor, *ctx->surface_stack}
{
}

void mf::WlrScreencopyManagerV1::capture_on_damage(WlrScreencopyV1DamageTracker::Frame* frame)
{
    damage_tracker.capture_on_damage(frame);
}

void mf::WlrScreencopyManagerV1::capture_output(
    wl_resource* frame,
    int32_t overlay_cursor,
    wl_resource* output)
{
    (void)overlay_cursor;
    auto const extents = OutputGlobal::from_or_throw(output).current_config().extents();
    new WlrScreencopyFrameV1{frame, this, ctx, {extents, output}};
}

void mf::WlrScreencopyManagerV1::capture_output_region(
    wl_resource* frame,
    int32_t overlay_cursor,
    wl_resource* output,
    int32_t x, int32_t y,
    int32_t width, int32_t height)
{
    (void)overlay_cursor;
    auto const extents = OutputGlobal::from_or_throw(output).current_config().extents();
    auto const intersection = geom::Rectangle{{x, y}, {width, height}}.intersection_with(extents);
    new WlrScreencopyFrameV1{frame, this, ctx, {intersection, output}};
}

mf::WlrScreencopyFrameV1::WlrScreencopyFrameV1(
    wl_resource* resource,
    WlrScreencopyManagerV1* manager,
    std::shared_ptr<WlrScreencopyV1Ctx> const& ctx,
    WlrScreencopyV1DamageTracker::FrameParams const& params)
    : wayland::WlrScreencopyFrameV1{resource, Version<3>()},
      manager{manager},
      ctx{ctx},
      params{params},
      stride{params.area.size.width.as_uint32_t() * 4}
{
    send_buffer_event(
        wayland::Shm::Format::argb8888,
        params.area.size.width.as_uint32_t(),
        params.area.size.height.as_uint32_t(),
        stride.as_uint32_t());
    send_buffer_done_event_if_supported();
}

void mf::WlrScreencopyFrameV1::capture(std::optional<geom::Rectangle> const& damage)
{
    if (!target)
    {
        fatal_error(
            "WlrScreencopyFrameV1::capture() called without a target, copy %s been called",
            copy_has_been_called ? "has" : "has not");
    }
    ctx->screen_shooter->capture(std::move(target), params.area,
        [wayland_executor=ctx->wayland_executor, damage, self=mw::make_weak(this)]
            (std::optional<time::Timestamp> captured_time)
        {
            wayland_executor->spawn([self, captured_time, damage]()
                {
                    if (self)
                    {
                        self.value().report_result(captured_time, damage);
                    }
                });
        });
}

void mf::WlrScreencopyFrameV1::prepare_target(wl_resource* buffer)
{
    if (copy_has_been_called)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::already_used,
            "Attempted to copy frame multiple times"));
    }
    copy_has_been_called = true;
    auto graphics_buffer = ctx->allocator->buffer_from_shm(buffer, ctx->wayland_executor, [](){});
    if (graphics_buffer->pixel_format() != mir_pixel_format_argb_8888)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid pixel format %d",
            graphics_buffer->pixel_format()));
    }
    if (graphics_buffer->size() != params.area.size)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid buffer size %dx%d, should be %dx%d",
            graphics_buffer->size().width.as_int(),
            graphics_buffer->size().height.as_int(),
            params.area.size.width.as_int(),
            params.area.size.height.as_int()));
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
    target = std::dynamic_pointer_cast<renderer::software::WriteMappableBuffer>(std::move(graphics_buffer));
    if (!target)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Failed to get write-mappable buffer out of Wayland SHM buffer"));
    }
}

void mf::WlrScreencopyFrameV1::report_result(
    std::optional<time::Timestamp> captured_time,
    std::optional<geom::Rectangle> const& damage)
{
    if (captured_time)
    {
        send_flags_event(Flags::y_invert);

        if (should_send_damage)
        {
            geom::Rectangle const damage_in_area{damage ?
                intersection_of(damage.value(), params.area) :
                params.area};
            geom::Rectangle const local_damage{
                damage_in_area.top_left - as_displacement(params.area.top_left),
                damage_in_area.size};
            send_damage_event(
                local_damage.top_left.x.as_uint32_t(),
                local_damage.top_left.y.as_uint32_t(),
                local_damage.size.width.as_uint32_t(),
                local_damage.size.height.as_uint32_t());
        }

        WaylandTimespec const timespec{captured_time.value()};
        send_ready_event(
            timespec.tv_sec_hi,
            timespec.tv_sec_lo,
            timespec.tv_nsec);
    }
    else
    {
        send_failed_event();
    }
}

void mf::WlrScreencopyFrameV1::copy(wl_resource* buffer)
{
    prepare_target(buffer);
    capture(std::nullopt);
}

void mf::WlrScreencopyFrameV1::copy_with_damage(wl_resource* buffer)
{
    prepare_target(buffer);
    should_send_damage = true;
    if (manager)
    {
        manager.value().capture_on_damage(this);
    }
    else
    {
        capture(std::nullopt);
    }
}
