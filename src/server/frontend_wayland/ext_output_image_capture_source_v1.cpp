/*
 * Copyright © Canonical Ltd.
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

#include "ext_output_image_capture_source_v1.h"

#include <mir/executor.h>
#include <mir/fatal.h>
#include <mir/compositor/screen_shooter.h>
#include <mir/compositor/screen_shooter_factory.h>
#include <mir/frontend/surface_stack.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/scene/scene_change_notification.h>
#include "ext_image_capture_v1.h"
#include "output_manager.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
auto translate_and_scale(geom::Rectangle rect, geom::Rectangle input_space, geom::Rectangle output_space)
    -> geom::Rectangle
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

namespace mir::frontend
{

namespace
{
struct ExtImageCaptureV1Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<compositor::ScreenShooterFactory> const screen_shooter_factory;
    std::shared_ptr<SurfaceStack> const surface_stack;
};
}

class ExtOutputImageCopyBackend : public ExtImageCopyBackend, public OutputConfigListener
{
public:
    ExtOutputImageCopyBackend(
        ExtImageCopyCaptureSessionV1* session,
        bool overlay_cursor,
        OutputGlobal* output,
        std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);
    ~ExtOutputImageCopyBackend();

    bool has_damage() override;
    void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
        CaptureCallback const& callback) override;

private:
    wayland::Weak<OutputGlobal> const output;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;

    wayland::DestroyListenerId destroy_listener_id;
    std::shared_ptr<scene::SceneChangeNotification> change_notifier;
    std::unique_ptr<compositor::ScreenShooter> const screen_shooter;

    void create_change_notifier();
    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;
};

class ExtOutputImageCaptureSourceManagerV1Global : public wayland::OutputImageCaptureSourceManagerV1::Global
{
public:
    ExtOutputImageCaptureSourceManagerV1Global(wl_display* display, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtOutputImageCaptureSourceManagerV1 : public wayland::OutputImageCaptureSourceManagerV1
{
public:
    ExtOutputImageCaptureSourceManagerV1(wl_resource* resource, std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void create_source(wl_resource* new_resource, wl_resource* output) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

}

mf::ExtOutputImageCopyBackend::ExtOutputImageCopyBackend(
    ExtImageCopyCaptureSessionV1* session,
    bool overlay_cursor,
    OutputGlobal* output,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    ExtImageCopyBackend{session, overlay_cursor},
    output{output},
    ctx{ctx},
    screen_shooter(ctx->screen_shooter_factory->create(thread_pool_executor))
{
    if (output)
    {
        output->add_listener(this);
        destroy_listener_id = output->add_destroy_listener([session]() { session->set_stopped(); });
        output_config_changed(output->current_config());
    }
    else
    {
        session->set_stopped();
    }
}

mf::ExtOutputImageCopyBackend::~ExtOutputImageCopyBackend()
{
    if (output)
    {
        output.value().remove_listener(this);
        output.value().remove_destroy_listener(destroy_listener_id);
    }
    if (change_notifier)
    {
        ctx->surface_stack->remove_observer(change_notifier);
    }
}

auto mf::ExtOutputImageCopyBackend::output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool
{
    // Send buffer constraints matching new output configuration
    auto const buffer_size = config.modes[config.current_mode_index].size;
    session->set_buffer_constraints(buffer_size);

    // Mark the whole buffer as damaged
    output_space_area = output.value().current_config().extents();
    apply_damage(std::nullopt);
    return true;
}

bool mf::ExtOutputImageCopyBackend::has_damage()
{
    if (!change_notifier)
    {
        // Create the change notifier the first time a client tries to
        // capture a frame.
        create_change_notifier();
    }

    return ExtImageCopyBackend::has_damage();
}

void mf::ExtOutputImageCopyBackend::create_change_notifier()
{
    auto callback = [this, weak_self = wayland::make_weak(this)](std::optional<geom::Rectangle> const& damage)
    {
        ctx->wayland_executor->spawn(
            [weak_self, damage]()
            {
                if (weak_self)
                {
                    weak_self.value().apply_damage(damage);
                }
            });
    };
    change_notifier = std::make_shared<ms::SceneChangeNotification>([callback]() { callback(std::nullopt); }, callback);
    ctx->surface_stack->add_observer(change_notifier);
}

void mf::ExtOutputImageCopyBackend::begin_capture(
    std::shared_ptr<renderer::software::RWMappable> const& shm_data,
    [[maybe_unused]] geom::Rectangle const& frame_damage,
    CaptureCallback const& callback)
{
    using FailureReason = wayland::ImageCopyCaptureFrameV1::FailureReason;
    if (!output)
    {
        callback(std::unexpected(FailureReason::stopped));
        return;
    }

    auto const& output_config = output.value().current_config();
    auto const buffer_size = output_config.modes[output_config.current_mode_index].size;
    auto const output_space_area = output_config.extents();
    auto const transform = output_config.transformation();

    // Check that the provided buffer matches our constraints
    if (!shm_data || shm_data->format() != mir_pixel_format_argb_8888 || shm_data->size() != buffer_size)
    {
        callback(std::unexpected(FailureReason::buffer_constraints));
        return;
    }

    geom::Rectangle buffer_space_damage;
    switch (damage_amount)
    {
    case DamageAmount::none:
        // This should never happen as maybe_capture_frame() checks has_damage() before calling begin_capture()
        MIR_FATAL_ERROR("begin_capture() called with no damage - this is a precondition violation");
        break;

    case DamageAmount::partial:
        buffer_space_damage = translate_and_scale(output_space_damage, output_space_area, {{}, buffer_size});
        break;

    case DamageAmount::full:
        buffer_space_damage = {{}, buffer_size};
        break;
    }

    // TODO: capture only the region covered by
    // union(buffer_space_damage, frame->frame_damage)

    screen_shooter->capture(
        shm_data,
        output_space_area,
        transform,
        overlay_cursor,
        [executor = ctx->wayland_executor, buffer_space_damage, callback](std::optional<time::Timestamp> captured_time)
        {
            executor->spawn(
                [callback, captured_time, buffer_space_damage]()
                {
                    if (captured_time)
                    {
                        callback({{captured_time.value(), buffer_space_damage}});
                    }
                    else
                    {
                        callback(std::unexpected(FailureReason::unknown));
                    }
                });
        });

    damage_amount = DamageAmount::none;
}

auto mf::create_ext_output_image_capture_source_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
    std::shared_ptr<SurfaceStack> const& surface_stack)
    -> std::shared_ptr<wayland::OutputImageCaptureSourceManagerV1::Global>
{
    auto ctx = std::make_shared<ExtImageCaptureV1Ctx>(wayland_executor, screen_shooter_factory, surface_stack);
    return std::make_shared<ExtOutputImageCaptureSourceManagerV1Global>(display, ctx);
}

mf::ExtOutputImageCaptureSourceManagerV1Global::ExtOutputImageCaptureSourceManagerV1Global(
    wl_display* display,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    Global{display, Version<1>()},
    ctx{ctx}
{}

void mf::ExtOutputImageCaptureSourceManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtOutputImageCaptureSourceManagerV1{new_resource, ctx};
}

mf::ExtOutputImageCaptureSourceManagerV1::ExtOutputImageCaptureSourceManagerV1(
    wl_resource* resource,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    wayland::OutputImageCaptureSourceManagerV1{resource, Version<1>()},
    ctx{ctx}
{}

void mf::ExtOutputImageCaptureSourceManagerV1::create_source(wl_resource* new_resource, wl_resource* output)
{
    auto& output_global = OutputGlobal::from_or_throw(output);
    ExtImageCopyBackendFactory backend_factory =
        [output = wayland::make_weak(&output_global), ctx = ctx](auto* session, bool overlay_cursor)
    {
        return std::make_shared<ExtOutputImageCopyBackend>(
            session, overlay_cursor, wayland::as_nullable_ptr(output), ctx);
    };
    ExtImageCopyCursorMapPosition cursor_map_position =
        [output = wayland::make_weak(&output_global)](float abs_x, float abs_y) -> std::optional<geom::Point>
    {
        if (!output)
        {
            return std::nullopt;
        }
        auto const config = output.value().current_config();
        auto const output_space_area = config.extents();
        geom::Point const abs_pos{abs_x, abs_y};
        if (!output_space_area.contains(abs_pos))
        {
            return std::nullopt;
        }

        // Translate pointer coordinates to buffer space. We don't take
        // rotations/flips into account because our image copy backend is
        // producing untransformed frames.
        auto const buffer_size = config.modes[config.current_mode_index].size;
        auto const x_scale =
            static_cast<double>(buffer_size.width.as_value()) / output_space_area.size.width.as_value();
        auto const y_scale =
            static_cast<double>(buffer_size.height.as_value()) / output_space_area.size.height.as_value();

        auto const raw_offset = abs_pos - output_space_area.top_left;
        decltype(raw_offset) const scaled_offset{x_scale * raw_offset.dx, y_scale * raw_offset.dy};
        return as_point(scaled_offset);
    };
    new ExtImageCaptureSourceV1{new_resource, backend_factory, cursor_map_position};
}
