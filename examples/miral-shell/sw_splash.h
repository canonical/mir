/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIRAL_SHELL_SW_SPLASH_H
#define MIRAL_SHELL_SW_SPLASH_H

#include <mir_toolkit/client_types.h>

#include <memory>

namespace mir { class Server; namespace scene { class Session; }}

// A very simple s/w rendered splash animation
class SwSplash
{
public:
    SwSplash();
    ~SwSplash();

    void operator()(MirConnection* connection);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);
    auto session() const -> std::shared_ptr<mir::scene::Session>;

private:
    struct Self;
    std::shared_ptr<Self> const self;
};

#endif //MIRAL_SHELL_SW_SPLASH_H
