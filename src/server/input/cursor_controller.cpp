/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/input/cursor_controller.h"

#include "mir/input/scene.h"
#include "mir/input/surface.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/scene/observer.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"

#include <exception>
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
        // Attribute changing alone won't trigger a cursor update
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
    void frame_posted(ms::Surface const*, geom::Rectangle const&) override
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
    void cursor_image_set_to(ms::Surface const*, std::weak_ptr<mir::graphics::CursorImage> const&) override
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
        surface->register_interest(observer);

        {
            std::unique_lock lg(surface_observers_guard);
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
            std::unique_lock lg(surface_observers_guard);
            auto it = surface_observers.find(surface.get());
            if (it != surface_observers.end())
            {
                surface->unregister_interest(*it->second);
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
        std::unique_lock lg(surface_observers_guard);
        for (auto &kv : surface_observers)
        {
                if (auto surface = kv.first)
                {
                    surface->unregister_interest(*kv.second);
                }
        }
        surface_observers.clear();
    }

private:
    mi::CursorController* const cursor_controller;

    std::mutex surface_observers_guard;
    std::map<ms::Surface*, std::shared_ptr<ms::SurfaceObserver>> surface_observers;
};

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
    cursor->hide(); // Cursor should be hidden unless there's a pointing device
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
    auto const usable = this->usable;

    lock.unlock();

    if (image && !is_empty(image))
    {
        if (usable)
            cursor->show(image);
    }
    else
        cursor->hide();
}

void mi::CursorController::update_cursor_image_locked(std::unique_lock<std::mutex>& lock)
{
    if (auto const surface = input_targets->input_surface_at(cursor_location))
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
    std::unique_lock lock(cursor_state_guard);
    update_cursor_image_locked(lock);
}

void mi::CursorController::cursor_moved_to(float abs_x, float abs_y)
{
    auto const new_location = geom::Point{geom::X{abs_x}, geom::Y{abs_y}};

    std::shared_ptr<ms::Surface> di;
    {
        std::unique_lock lock(cursor_state_guard);

        cursor_location = new_location;

        update_cursor_image_locked(lock);
        di = drag_icon.lock();
    }

    cursor->move_to(new_location);
    if (di)
    {
        di->move_to(new_location);
    }
}

void mir::input::CursorController::pointer_usable()
{
    std::lock_guard lock(serialize_pointer_usable_unusable);
    bool became_usable = false;
    std::shared_ptr<mg::CursorImage> image;
    std::shared_ptr<ms::Surface> di;

    {
        std::lock_guard lock(cursor_state_guard);
        became_usable = !usable;
        image = current_cursor;
        usable = true;
        di = drag_icon.lock();
    }

    if (became_usable)
    {
        if (image && !is_empty(image))
            cursor->show(image);
        if (di)
        {
            di->show();
        }
    }
}

void mir::input::CursorController::pointer_unusable()
{
    std::lock_guard lock(serialize_pointer_usable_unusable);
    std::shared_ptr<ms::Surface> di;

    {
        std::lock_guard lock(cursor_state_guard);
        usable = false;
        di = drag_icon.lock();
    }

    cursor->hide();
    if (di)
    {
        di->hide();
    }
}

void mir::input::CursorController::set_drag_icon(std::weak_ptr<scene::Surface> icon)
{
    std::shared_ptr<ms::Surface> di;

    {
        std::lock_guard lock{cursor_state_guard};
        drag_icon = std::move(icon);

        di = drag_icon.lock();
    }

    if (di)
    {
        di->move_to(cursor_location);
    }
}
