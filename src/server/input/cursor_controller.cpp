/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "cursor_controller.h"

#include "mir/input/input_targets.h"
#include "mir/input/surface.h"
#include "mir/graphics/cursor.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface.h"

#include <functional>
#include <mutex>
#include <map>

#include <assert.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{

struct SurfaceObserver : ms::SurfaceObserver
{
    SurfaceObserver(std::function<void()> const& update_cursor)
        : update_cursor(update_cursor)
    {
    }

    void attrib_changed(MirSurfaceAttrib, int)
    {
        // Attribute changing alone wont trigger a cursor update
    }
    void resized_to(geom::Size const&)
    {
        update_cursor();
    }
    void moved_to(geom::Point const&)
    {
        update_cursor();
    }
    void hidden_set_to(bool)
    {
        update_cursor();
    }
    void frame_posted()
    {
        // Frame posting wont trigger a cursor update
    }
    void alpha_set_to(float)
    {
        update_cursor();
    }
    void transformation_set_to(glm::mat4 const&)
    {
        update_cursor();
    }
    void reception_mode_set_to(mi::InputReceptionMode)
    {
        update_cursor();
    }
    void cursor_image_set_to(mg::CursorImage const&)
    {
        update_cursor();
    }

    std::function<void()> const update_cursor;
};

struct Observer : ms::Observer
{
    Observer(std::function<void()> const& update_cursor)
        : update_cursor(update_cursor)
    {
    }
    
    void add_surface_observer(ms::Surface* surface)
    {
        auto observer = std::make_shared<SurfaceObserver>(update_cursor);
        surface->add_observer(observer);

        {
            std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
            surface_observers[surface] = observer;
        }
    }
    
    void surface_added(ms::Surface *surface)
    {
        add_surface_observer(surface);
        update_cursor();
    }
    void surface_removed(ms::Surface *surface)
    {
        {
            std::unique_lock<decltype(surface_observers_guard)> lg(surface_observers_guard);
            auto it = surface_observers.find(surface);
            if (it != surface_observers.end())
            {
                surface->remove_observer(it->second);
                surface_observers.erase(it);
            }
        }
        update_cursor();
    }
    void surfaces_reordered()
    {
        update_cursor();
    }
    
    void surface_exists(ms::Surface *surface)
    {
        add_surface_observer(surface);
        update_cursor();
    }
    
    void end_observation()
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
    
    std::function<void()> const update_cursor;

    std::mutex surface_observers_guard;
    std::map<ms::Surface*, std::weak_ptr<SurfaceObserver>> surface_observers;
};

std::shared_ptr<mi::Surface> topmost_surface_containing_point(
    std::shared_ptr<mi::InputTargets> const& targets, geom::Point const& point)
{
    std::shared_ptr<mi::Surface> top_surface_at_point = nullptr;
    targets->for_each([&top_surface_at_point, &point]
        (std::shared_ptr<mi::Surface> const& surface) 
        {
            if (surface->input_area_contains(point))
                top_surface_at_point = surface;
        });
    return top_surface_at_point;
}

}

mi::CursorController::CursorController(std::shared_ptr<mi::InputTargets> const& input_targets,
    std::shared_ptr<mg::Cursor> const& cursor,
    std::shared_ptr<mg::CursorImage> const& default_cursor_image) :
        input_targets(input_targets),
        cursor(cursor),
        default_cursor_image(default_cursor_image),
        current_cursor(default_cursor_image)
{
    // TODO: Add observer could return weak_ptr to eliminate this
    // pattern
    auto strong_observer = std::make_shared<Observer>([&]()
    {
        std::lock_guard<std::mutex> lg(cursor_state_guard);
        update_cursor_image_locked(lg);
    });
    input_targets->add_observer(strong_observer);
    observer = strong_observer;
}

mi::CursorController::~CursorController()
{
    input_targets->remove_observer(observer);
}

void mi::CursorController::set_cursor_image_locked(std::lock_guard<std::mutex> const&,
    std::shared_ptr<mg::CursorImage> const& image)
{
    if (current_cursor == image)
    {
        return;
    }

    current_cursor = image;
    if (image)
        cursor->show(*image);
    else
        cursor->hide();
}

void mi::CursorController::update_cursor_image_locked(std::lock_guard<std::mutex> const& lg)
{
    auto surface = topmost_surface_containing_point(input_targets, cursor_location);
    if (surface)
    {
        set_cursor_image_locked(lg, surface->cursor_image());
    }
    else
    {
        set_cursor_image_locked(lg, default_cursor_image);
    }
}

void mi::CursorController::cursor_moved_to(float abs_x, float abs_y)
{
    std::lock_guard<std::mutex> lg(cursor_state_guard);

    assert(input_targets);

    cursor_location = geom::Point{geom::X{abs_x}, geom::Y{abs_y}};
    
    update_cursor_image_locked(lg);

    cursor->move_to(cursor_location);
}
