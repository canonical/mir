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
#include "mir/wayland/weak.h"
#include "mir/wayland/protocol_error.h"
#include "mir/executor.h"
#include "wayland_wrapper.h"
#include "wayland_timespec.h"
#include "output_manager.h"
#include "shm.h"

#include <boost/throw_exception.hpp>
#include <mutex>
#include <optional>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
auto translate_and_scale(
    geom::Rectangle rect,
    geom::Rectangle input_space,
    geom::Rectangle output_space) -> geom::Rectangle
{
    auto const x_scale = static_cast<double>(output_space.size.width.as_value()) / input_space.size.width.as_value();
    auto const y_scale = static_cast<double>(output_space.size.height.as_value()) / input_space.size.height.as_value();
    rect.size.width = rect.size.width * x_scale;
    rect.size.height = rect.size.height * y_scale;
    auto displacement = rect.top_left - input_space.top_left;
    rect.top_left.x = output_space.top_left.x + displacement.dx * x_scale;
    rect.top_left.y = output_space.top_left.y + displacement.dy * y_scale;
    return rect;
}
}

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
    Area(Area const&) = delete;
    Area& operator=(Area const&) = delete;

    void capture_frame();

    /// The amount of damage since the last frame was captured (should be none unless pending_frame is null)
    DamageAmount damage_amount;
    /// Only has meaning when damage_amount is partial
    geom::Rectangle output_space_damage;
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
    void capture(geometry::Rectangle buffer_space_damage) override;
    /// @}

private:
    void prepare_target(wl_resource* buffer);
    void report_result(std::optional<time::Timestamp> captured_time, geom::Rectangle buffer_space_damage);

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
        [&](auto& area){ return area->params == frame_params; });
    if (area != end(areas))
    {
        (*area)->add_frame(frame);
    }
    else
    {
        // We capture the given frame immediately, and also push an empty capture area so that if we get another
        // capture request with the same params it will wait for damage since this frame.
        frame->capture(frame_params.full_buffer_space_damage());
        // If an unusually high number of capture areas have been created for some reason, clear the list rather than
        // getting bogged down (it's ok, worst case scenario waiting for damage doesn't work)
        if (areas.size() > 100)
        {
            areas.clear();
        }
        areas.push_back(std::make_unique<Area>(frame_params));
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
                            area->apply_damage(damage);
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
        auto const intersection = intersection_of(damage.value(), params.output_space_area);
        if (intersection.size != geom::Size{})
        {
            if (damage_amount == DamageAmount::partial)
            {
                output_space_damage = geom::Rectangles{output_space_damage, intersection}.bounding_rectangle();
            }
            else // damage_amount == DamageAmount::none
            {
                damage_amount = DamageAmount::partial;
                output_space_damage = intersection;
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
        // Capture with a zero-sized damage rect
        pending_frame.value().capture({});
    }   break;

    case DamageAmount::partial:
        pending_frame.value().capture(translate_and_scale(
            output_space_damage,
            params.output_space_area,
            {{}, params.buffer_size}));
        break;

    case DamageAmount::full:
        pending_frame.value().capture(params.full_buffer_space_damage());
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
    auto const& output_config = OutputGlobal::from_or_throw(output).current_config();
    auto const extents = output_config.extents();
    auto const buffer_size = output_config.modes[output_config.current_mode_index].size;
    new WlrScreencopyFrameV1{frame, this, ctx, {output, extents, buffer_size}};
}

void mf::WlrScreencopyManagerV1::capture_output_region(
    wl_resource* frame,
    int32_t overlay_cursor,
    wl_resource* output,
    int32_t x, int32_t y,
    int32_t width, int32_t height)
{
    (void)overlay_cursor;
    auto const& output_config = OutputGlobal::from_or_throw(output).current_config();
    auto const extents = output_config.extents();
    auto const intersection = intersection_of({{x, y}, {width, height}}, extents);
    auto const output_size = output_config.modes[output_config.current_mode_index].size;
    auto const buffer_size = translate_and_scale(intersection, extents, {{}, output_size}).size;
    new WlrScreencopyFrameV1{frame, this, ctx, {output, intersection, buffer_size}};
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
      stride{params.buffer_size.width.as_uint32_t() * 4}
{
    send_buffer_event(
        wayland::Shm::Format::argb8888,
        params.buffer_size.width.as_uint32_t(),
        params.buffer_size.height.as_uint32_t(),
        stride.as_uint32_t());
    send_buffer_done_event_if_supported();
}

void mf::WlrScreencopyFrameV1::capture(geom::Rectangle buffer_space_damage)
{
    if (!target)
    {
        fatal_error(
            "WlrScreencopyFrameV1::capture() called without a target, copy %s been called",
            copy_has_been_called ? "has" : "has not");
    }
    ctx->screen_shooter->capture(std::move(target), params.output_space_area,
        [wayland_executor=ctx->wayland_executor, buffer_space_damage, self=mw::make_weak(this)]
            (std::optional<time::Timestamp> captured_time)
        {
            wayland_executor->spawn([self, captured_time, buffer_space_damage]()
                {
                    if (self)
                    {
                        self.value().report_result(captured_time, buffer_space_damage);
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
    auto shm_buffer = mf::ShmBuffer::from(buffer);
    if (!shm_buffer)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Copy target is not a wl_shm buffer"));
    }
    auto shm_data = shm_buffer->data();
    if (shm_data->format() != mir_pixel_format_argb_8888)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid pixel format %d",
            shm_data->format()));
    }
    if (shm_data->size() != params.buffer_size)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid buffer size %dx%d, should be %dx%d",
            shm_data->size().width.as_int(),
            shm_data->size().height.as_int(),
            params.buffer_size.width.as_int(),
            params.buffer_size.height.as_int()));
    }
    if (shm_data->stride() != stride)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            Error::invalid_buffer,
            "Invalid stride %d, should be %d",
            shm_data->stride().as_int(),
            stride.as_int()));
    }

    target = std::shared_ptr<mir::renderer::software::WriteMappableBuffer>{
        shm_data.get(),
        [shm_data, weak_buffer = mw::make_weak(shm_buffer), executor = ctx->wayland_executor](auto*)
        {
            // Send release event for underlying buffer if necessary
            executor->spawn(
                [shm_data, weak_buffer]()
                {
                    if (weak_buffer)
                    {
                        weak_buffer.value().send_release_event();
                    }
                });
        }
    };
}

void mf::WlrScreencopyFrameV1::report_result(
    std::optional<time::Timestamp> captured_time,
    geom::Rectangle buffer_space_damage)
{
    if (captured_time)
    {
        send_flags_event(Flags::y_invert);

        if (should_send_damage)
        {
            send_damage_event(
                buffer_space_damage.top_left.x.as_uint32_t(),
                buffer_space_damage.top_left.y.as_uint32_t(),
                buffer_space_damage.size.width.as_uint32_t(),
                buffer_space_damage.size.height.as_uint32_t());
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
    capture(params.full_buffer_space_damage());
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
        capture(params.full_buffer_space_damage());
    }
}
