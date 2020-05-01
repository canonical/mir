/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "cursor_controller.h"

#include "mir/input/scene.h"
#include "mir/input/surface.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/scene/observer.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"

#include <mutex>
#include <map>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{

struct UpdateCursorOnSurfaceChanges : ms::NullSurfaceObserver
{
    UpdateCursorOnSurfaceChanges(mi::CursorController* cursor_controller)
        : cursor_controller(cursor_controller)
    {
    }

    void attrib_changed(ms::Surface const*, MirWindowAttrib, int) override
    {
        // Attribute changing alone wont trigger a cursor update
    }
    void content_resized_to(ms::Surface const*, geom::Size const&) override
    {
        cursor_controller->update_cursor_image();
    }
    void moved_to(ms::Surface const*, geom::Point const&) override
    {
        cursor_controller->update_cursor_image();
    }
    void hidden_set_to(ms::Surface const*, bool) override
    {
        cursor_controller->update_cursor_image();
    }
    void frame_posted(ms::Surface const*, int, geom::Size const&) override
    {
        // The first frame posted will trigger a cursor update, since it
        // changes the visibility status of the surface, and can thus affect
        // the cursor.
        if (!first_frame_posted)
        {
            first_frame_posted = true;
            cursor_controller->update_cursor_image();
        }
    }
    void alpha_set_to(ms::Surface const*, float) override
    {
        cursor_controller->update_cursor_image();
    }
    void transformation_set_to(ms::Surface const*, glm::mat4 const&) override
    {
        cursor_controller->update_cursor_image();
    }
    void reception_mode_set_to(ms::Surface const*, mi::InputReceptionMode) override
    {
        cursor_controller->update_cursor_image();
    }
    void cursor_image_set_to(ms::Surface const*, const mir::graphics::CursorImage&) override
    {
        cursor_controller->update_cursor_image();
    }
    void cursor_image_removed(ms::Surface const*) override
    {
        cursor_controller->update_cursor_image();
    }
    void orientation_set_to(ms::Surface const*, MirOrientation) override
    {
        // No need to update cursor for orientation property change alone.
    }
    void client_surface_close_requested(ms::Surface const*) override
    {
        // No need to update cursor for client close requests
    }

    mi::CursorController* const cursor_controller;
    bool first_frame_posted = false;
};

struct UpdateCursorOnSceneChanges : ms::Observer
{
    UpdateCursorOnSceneChanges(mi::CursorController* cursor_controller)
        : cursor_controller(cursor_controller)
    {
    }

    void add_surface_observer(ms::Surface* surface)
    {
        auto const observer = std::make_shared<UpdateCursorOnSurfaceChanges>(cursor_controller);
        surface->add_observer(observer);

        {
            std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
            surface_observers[surface] = observer;
        }
    }

    void surface_added(std::shared_ptr<ms::Surface> const& surface) override
    {
        add_surface_observer(surface.get());
        cursor_controller->update_cursor_image();
    }

    void surface_removed(std::shared_ptr<ms::Surface> const& surface) override
    {
        {
            std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
            auto it = surface_observers.find(surface.get());
            if (it != surface_observers.end())
            {
                surface->remove_observer(it->second);
                surface_observers.erase(it);
            }
        }
        cursor_controller->update_cursor_image();
    }

    void surfaces_reordered(ms::SurfaceSet const&) override
    {
        cursor_controller->update_cursor_image();
    }

    void scene_changed() override
    {
        cursor_controller->update_cursor_image();
    }

    void surface_exists(std::shared_ptr<ms::Surface> const& surface) override
    {
        add_surface_observer(surface.get());
        cursor_controller->update_cursor_image();
    }

    void end_observation() override
    {
        std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
        for (auto &kv : surface_observers)
        {
                auto surface = kv.first;
                if (surface)
                    surface->remove_observer(kv.second);
        }
        surface_observers.clear();
    }

private:
    mi::CursorController* const cursor_controller;

    std::mutex surface_observers_guard;
    std::map<ms::Surface*, std::weak_ptr<ms::SurfaceObserver>> surface_observers;
};

std::shared_ptr<mi::Surface> topmost_surface_containing_point(
    std::shared_ptr<mi::Scene> const& targets, geom::Point const& point)
{
    std::shared_ptr<mi::Surface> top_surface_at_point;
    targets->for_each([&top_surface_at_point, &point]
        (std::shared_ptr<mi::Surface> const& surface) 
        {
            if (surface->input_area_contains(point))
                top_surface_at_point = surface;
        });
    return top_surface_at_point;
}

bool is_empty(std::shared_ptr<mg::CursorImage> const& image)
{
    auto const size = image->size();
    return size.width.as_int() == 0 || size.height.as_int() == 0;
}
}

mi::CursorController::CursorController(std::shared_ptr<mi::Scene> const& input_targets,
    std::shared_ptr<mg::Cursor> const& cursor,
    std::shared_ptr<mg::CursorImage> const& default_cursor_image) :
        input_targets(input_targets),
        cursor(cursor),
        default_cursor_image(default_cursor_image),
        current_cursor(default_cursor_image)
{
    // TODO: Add observer could return weak_ptr to eliminate this
    // pattern
    auto strong_observer = std::make_shared<UpdateCursorOnSceneChanges>(this);
    input_targets->add_observer(strong_observer);
    observer = strong_observer;
}

mi::CursorController::~CursorController()
{
    try 
    {
        input_targets->remove_observer(observer);
    }
    catch (...)
    {
        std::terminate();
    }
}

void mi::CursorController::set_cursor_image_locked(std::unique_lock<std::mutex>& lock,
    std::shared_ptr<mg::CursorImage> const& image)
{
    if (current_cursor == image)
    {
        return;
    }

    current_cursor = image;

    lock.unlock();

    if (image && !is_empty(image))
        cursor->show(*image);
    else
        cursor->hide();
}

void mi::CursorController::update_cursor_image_locked(std::unique_lock<std::mutex>& lock)
{
    auto surface = topmost_surface_containing_point(input_targets, cursor_location);
    if (surface)
    {
        set_cursor_image_locked(lock, surface->cursor_image());
    }
    else
    {
        set_cursor_image_locked(lock, default_cursor_image);
    }
}

void mi::CursorController::update_cursor_image()
{
    std::unique_lock<std::mutex> lock(cursor_state_guard);
    update_cursor_image_locked(lock);
}

void mi::CursorController::cursor_moved_to(float abs_x, float abs_y)
{
    auto const new_location = geom::Point{geom::X{abs_x}, geom::Y{abs_y}};

    {
        std::unique_lock<std::mutex> lock(cursor_state_guard);

        cursor_location = new_location;

        update_cursor_image_locked(lock);
    }

    cursor->move_to(new_location);
}
