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

#include "ext_foreign_toplevel_image_capture_source_v1.h"

#include <mir/executor.h>
#include <mir/compositor/screen_shooter.h>
#include <mir/compositor/screen_shooter_factory.h>
#include <mir/frontend/surface_stack.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/scene/null_observer.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/surface.h>
#include <mir/scene/surface_scene.h>
#include "ext_image_capture_v1.h"
#include "foreign_toplevel_list_v1.h"
#include "mir/scene/null_observer.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;

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

class ExtForeignToplevelImageCopyBackend : public ExtImageCopyBackend
{
public:
    ExtForeignToplevelImageCopyBackend(
        ExtImageCopyCaptureSessionV1* session,
        bool overlay_cursor,
        std::weak_ptr<scene::Surface> surface,
        std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);
    ~ExtForeignToplevelImageCopyBackend();

    bool has_damage() override;
    void begin_capture(
        std::shared_ptr<renderer::software::RWMappable> const& shm_data,
        geom::Rectangle const& frame_damage,
        CaptureCallback const& callback) override;

private:
    class SceneObserver;
    class SurfaceObserver;

    void surface_removed();
    void surface_resized();
    void surface_updated(geom::Rectangle const& damage);

    std::weak_ptr<scene::Surface> const surface;
    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;

    std::shared_ptr<SceneObserver> scene_observer;
    std::shared_ptr<SurfaceObserver> surface_observer;
    std::unique_ptr<compositor::ScreenShooter> const screen_shooter;
};

class ExtForeignToplevelImageCaptureSourceManagerV1Global
    : public wayland::ForeignToplevelImageCaptureSourceManagerV1::Global
{
public:
    ExtForeignToplevelImageCaptureSourceManagerV1Global(
        wl_display* display,
        std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

class ExtForeignToplevelImageCaptureSourceManagerV1 : public wayland::ForeignToplevelImageCaptureSourceManagerV1
{
public:
    ExtForeignToplevelImageCaptureSourceManagerV1(
        wl_resource* resource,
        std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx);

private:
    void create_source(wl_resource* new_resource, wl_resource* toplevel_handle) override;

    std::shared_ptr<ExtImageCaptureV1Ctx> const ctx;
};

} // namespace mir::frontend

class mf::ExtForeignToplevelImageCopyBackend::SceneObserver : public ms::NullObserver,
                                                              public std::enable_shared_from_this<SceneObserver>
{
public:
    SceneObserver(ExtForeignToplevelImageCopyBackend* backend) : backend{backend} {}

    void surface_removed(std::shared_ptr<ms::Surface> const& surface) override
    {
        backend->ctx->wayland_executor->spawn(
            [weak_observer = weak_from_this(), weak_surface = std::weak_ptr<ms::Surface>(surface)]()
            {
                if (auto const observer = weak_observer.lock())
                {
                    if (auto const surface = weak_surface.lock())
                    {
                        if (surface == observer->backend->surface.lock())
                        {
                            observer->backend->surface_removed();
                        }
                    }
                }
            });
    }

private:
    ExtForeignToplevelImageCopyBackend* const backend;
};

class mf::ExtForeignToplevelImageCopyBackend::SurfaceObserver : public ms::NullSurfaceObserver
{
public:
    SurfaceObserver(ExtForeignToplevelImageCopyBackend* backend) : backend{backend} {}

    void window_resized_to(ms::Surface const*, geom::Size const&) override { backend->surface_resized(); }
    void frame_posted(ms::Surface const*, geom::Rectangle const& damage) override { backend->surface_updated(damage); }

private:
    ExtForeignToplevelImageCopyBackend* const backend;
};

mf::ExtForeignToplevelImageCopyBackend::ExtForeignToplevelImageCopyBackend(
    ExtImageCopyCaptureSessionV1* session,
    bool overlay_cursor,
    std::weak_ptr<scene::Surface> surface,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    ExtImageCopyBackend{session, overlay_cursor},
    surface{surface},
    ctx{ctx},
    scene_observer{std::make_shared<SceneObserver>(this)},
    surface_observer{std::make_shared<SurfaceObserver>(this)},
    screen_shooter{
        ctx->screen_shooter_factory->create_for_scene(thread_pool_executor, scene::create_surface_scene(surface))}
{
    if (auto const locked = surface.lock())
    {
        ctx->surface_stack->add_observer(scene_observer);
        locked->register_interest(surface_observer, *ctx->wayland_executor);
        surface_resized();
        surface_updated({});
    }
    else
    {
        surface_removed();
    }
}

mf::ExtForeignToplevelImageCopyBackend::~ExtForeignToplevelImageCopyBackend()
{
    if (auto const locked = surface.lock())
    {
        locked->unregister_interest(*surface_observer);
    }
    ctx->surface_stack->remove_observer(scene_observer);
}

void mf::ExtForeignToplevelImageCopyBackend::surface_removed() { session->set_stopped(); }

void mf::ExtForeignToplevelImageCopyBackend::surface_resized()
{
    if (auto const locked = surface.lock())
    {
        auto const window_size = locked->window_size();
        session->set_buffer_constraints(window_size);

        output_space_area = {{}, window_size};
        apply_damage(std::nullopt);
    }
}

void mf::ExtForeignToplevelImageCopyBackend::surface_updated(geom::Rectangle const& damage) { apply_damage(damage); }

bool mf::ExtForeignToplevelImageCopyBackend::has_damage() { return ExtImageCopyBackend::has_damage(); }

void mf::ExtForeignToplevelImageCopyBackend::begin_capture(
    std::shared_ptr<renderer::software::RWMappable> const& shm_data,
    [[maybe_unused]] geom::Rectangle const& frame_damage,
    CaptureCallback const& callback)
{
    using FailureReason = wayland::ImageCopyCaptureFrameV1::FailureReason;
    auto const locked = surface.lock();
    if (!locked)
    {
        callback(std::unexpected(FailureReason::stopped));
        return;
    }

    auto const buffer_size = locked->window_size();
    geom::Rectangle const output_space_area{locked->top_left(), buffer_size};
    glm::mat4 const transform{1};

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
        fatal_error_abort("begin_capture() called with no damage - this is a precondition violation");
        break;

    case DamageAmount::partial:
        // TODO: this will need to be scaled if we want to capture the
        // surface at it's native buffer resolution
        buffer_space_damage = output_space_damage;
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

auto mf::create_ext_foreign_toplevel_image_capture_source_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
    std::shared_ptr<SurfaceStack> const& surface_stack)
    -> std::shared_ptr<wayland::ForeignToplevelImageCaptureSourceManagerV1::Global>
{
    auto ctx = std::make_shared<ExtImageCaptureV1Ctx>(wayland_executor, screen_shooter_factory, surface_stack);
    return std::make_shared<ExtForeignToplevelImageCaptureSourceManagerV1Global>(display, ctx);
}

mf::ExtForeignToplevelImageCaptureSourceManagerV1Global::ExtForeignToplevelImageCaptureSourceManagerV1Global(
    wl_display* display,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    Global{display, Version<1>()},
    ctx{ctx}
{}

void mf::ExtForeignToplevelImageCaptureSourceManagerV1Global::bind(wl_resource* new_resource)
{
    new ExtForeignToplevelImageCaptureSourceManagerV1{new_resource, ctx};
}

mf::ExtForeignToplevelImageCaptureSourceManagerV1::ExtForeignToplevelImageCaptureSourceManagerV1(
    wl_resource* resource,
    std::shared_ptr<ExtImageCaptureV1Ctx> const& ctx) :
    wayland::ForeignToplevelImageCaptureSourceManagerV1{resource, Version<1>()},
    ctx{ctx}
{}

void mf::ExtForeignToplevelImageCaptureSourceManagerV1::create_source(
    wl_resource* new_resource,
    wl_resource* toplevel_handle)
{
    auto& toplevel = ExtForeignToplevelHandleV1::from_or_throw(toplevel_handle);
    auto const weak_surface = toplevel.surface();

    ExtImageCopyBackendFactory backend_factory = [weak_surface, ctx = ctx](auto* session, bool overlay_cursor)
    { return std::make_shared<ExtForeignToplevelImageCopyBackend>(session, overlay_cursor, weak_surface, ctx); };
    ExtImageCopyCursorMapPosition cursor_map_position =
        [weak_surface](float abs_x, float abs_y) -> std::optional<geom::Point>
    {
        auto const surface = weak_surface.lock();
        if (!surface)
        {
            return std::nullopt;
        }
        geom::Rectangle output_space_area{surface->top_left(), surface->window_size()};
        geom::Point const abs_pos{abs_x, abs_y};
        if (!output_space_area.contains(abs_pos))
        {
            return std::nullopt;
        }

        // TODO: The pointer position will need to be scaled if we
        // start rendering at the buffer's scale.
        return as_point(abs_pos - surface->top_left());
    };
    new ExtImageCaptureSourceV1{new_resource, backend_factory, cursor_map_position};
}
