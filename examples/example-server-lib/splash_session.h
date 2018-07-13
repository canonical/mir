/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SPLASH_SESSION_H
#define MIR_SPLASH_SESSION_H

#include <memory>

namespace mir { class Server; namespace scene { class Session; }}

class SplashSession
{
public:
    virtual auto session() const -> std::shared_ptr<mir::scene::Session> = 0;

    SplashSession() = default;
    virtual ~SplashSession() = default;
    SplashSession(SplashSession const&) = delete;
    SplashSession& operator=(SplashSession const&) = delete;
};

#endif //MIR_SPLASH_SESSION_H
