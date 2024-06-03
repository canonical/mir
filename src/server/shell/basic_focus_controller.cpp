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

#include "basic_focus_controller.h"
#include <mir/compositor/scene.h>
#include <mir/scene/surface.h>
#include <mir/scene/session.h>
#include <algorithm>

namespace msh = mir::shell;
namespace ms = mir::scene;


msh::BasicFocusControllerObserver::BasicFocusControllerObserver()
{
}
void msh::BasicFocusControllerObserver::surface_added(std::shared_ptr<ms::Surface> const& surface) 
{
    focus_order.push_back(surface);
}
void msh::BasicFocusControllerObserver::surface_removed(std::shared_ptr<ms::Surface> const& surface) 
{
    focus_order.erase(std::remove(focus_order.begin(), focus_order.end(), surface), focus_order.end());
}
void msh::BasicFocusControllerObserver::surfaces_reordered(ms::SurfaceSet const&)  {}
void msh::BasicFocusControllerObserver::scene_changed()  {}
void msh::BasicFocusControllerObserver::surface_exists(std::shared_ptr<ms::Surface> const&)  {}
void msh::BasicFocusControllerObserver::end_observation() 
{
    focus_order.clear();
}

std::shared_ptr<ms::Surface> msh::BasicFocusControllerObserver::focused_surface() const
{
    return focus_order.empty() ? nullptr : focus_order.front();
}

bool msh::BasicFocusControllerObserver::is_active() const
{
    return raised != nullptr;
}

bool msh::BasicFocusControllerObserver::is_focusable(std::shared_ptr<scene::Surface> const& surface) const
{
    auto current_focused = focused_surface();
    auto const current_windows_focus_mode = surface ? current_focused->focus_mode() : mir_focus_mode_focusable;
    if (current_windows_focus_mode == mir_focus_mode_grabbing)
        return false;

    // Check if the new window selection has selection disabled
    if (surface->focus_mode() == mir_focus_mode_disabled)
        return false;

    // Check if we can activate it at all.
    if (desired_window_selection.can_be_active() && desired_window_selection.state() != mir_window_state_hidden)
        return true;

    return false;
}

void msh::BasicFocusControllerObserver::advance(bool within_app)
{
    bool reverse = false;
    if (focus_order.empty())
        return;

    if (!is_active())
    {
        originally_selected_it = focus_order.begin();
        raised = focused_surface();
    }

    // Attempt to focus the next application after the selected application.
    auto it = std::find(focus_order.begin(), focus_order.end(), raised);

    std::optional<std::shared_ptr<mir::scene::Surface>> next_window = std::nullopt;
    do {
        if (reverse)
        {
            if (it == focus_order.begin())
                it = focus_order.end() - 1;
            else
                it--;
        }
        else
        {
            it++;
            if (it == focus_order.end())
                it = focus_order.begin();
        }

        // We made it all the way through the list but failed to find anything.
        // This means that there is no other selectable window in the list but
        // the currently selected one, so we don't need to select anything.
        if (*it == raised)
            return;

        auto session = it->get()->session().lock();
        if (within_app)
        {
            if (session == originally_selected_it->get()->session().lock() && tools.can_select_window(*it))
                next_window = *it;
            else
                next_window = std::nullopt;
        }
        else
        {
            // Check if we've already encountered this application. If so, we shouldn't try to select it again
            bool already_encountered = false;
            for (auto prev_it = originally_selected_it; prev_it != it; prev_it++)
            {
                if (prev_it == focus_order.end())
                    prev_it = focus_order.begin();

                if (session == prev_it->get()->session().lock())
                {
                    next_window = std::nullopt;
                    already_encountered = true;
                    break;
                }
            }
            if (!already_encountered)
                next_window = tools.window_to_select_application(it->application());
        }
    } while (next_window == std::nullopt);

    if (next_window == std::nullopt)
        return;

    if (it == originally_selected_it)
    {
        // Edge case: if we have gone full circle around the list back to the original app
        // then we will wind up in a situation where the original app is in the 2nd position
        // in the list, while the last app is in the 1st position. Hence, we send the selected
        // app to the back of the list.
        auto application = selected.application();
        for (auto& window: tools.info_for(application).windows())
            tools.send_tree_to_back(window);
    }
    else
        tools.swap_tree_order(next_window.value(), selected);

    auto next_state_to_preserve =  tools.info_for(next_window.value()).state();
    tools.select_active_window(next_window.value());
    restore_state = next_state_to_preserve;
    return next_window.value();
}

msh::BasicFocusController::BasicFocusController(std::shared_ptr<compositor::Scene> const& scene)
    : scene{scene},
      observer{std::make_shared<BasicFocusControllerObserver>()}
{
    scene->add_observer(observer);
}

msh::BasicFocusController::~BasicFocusController()
{
    scene->remove_observer(observer);
}

void msh::BasicFocusController::focus_next_session()
{

}