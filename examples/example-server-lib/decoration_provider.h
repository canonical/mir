/*
 * Copyright © 2016-2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SHELL_DECORATION_PROVIDER_H
#define MIRAL_SHELL_DECORATION_PROVIDER_H


#include <miral/window_manager_tools.h>

#include <condition_variable>
#include <mutex>

class DecorationProvider
{
public:
    DecorationProvider(miral::WindowManagerTools const& tools);
    ~DecorationProvider();

    void operator()(struct wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    auto session() const -> std::shared_ptr<mir::scene::Session>;

    void stop();

    bool is_decoration(miral::Window const& window) const;

private:
    struct Self;
    std::shared_ptr<Self> const self;

    std::mutex mutable mutex;
    bool running{false};
    std::condition_variable running_cv;
    std::weak_ptr<mir::scene::Session> weak_session;
};


#endif //MIRAL_SHELL_DECORATION_PROVIDER_H
