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

#ifndef MIRAL_SHELL_SPINNER_SPLASH_H
#define MIRAL_SHELL_SPINNER_SPLASH_H

#include "splash_session.h"

class SpinnerSplash
{
public:
    SpinnerSplash();
    ~SpinnerSplash();

    void operator()(struct wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    operator std::shared_ptr<SplashSession>() const;

private:
    struct Self;
    std::shared_ptr<Self> const self;
};

#endif //MIRAL_SHELL_SPINNER_SPLASH_H
