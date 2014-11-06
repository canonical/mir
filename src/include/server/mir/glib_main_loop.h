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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GLIB_MAIN_LOOP_H_
#define MIR_GLIB_MAIN_LOOP_H_

#include "mir/main_loop.h"

#include <atomic>

#include <glib.h>

namespace mir
{

namespace detail
{

class GMainContextHandle
{
public:
    GMainContextHandle();
    ~GMainContextHandle();
    operator GMainContext*() const;
private:
    GMainContext* const main_context;
};

}

class GLibMainLoop
{
public:
    GLibMainLoop();

    void run();
    void stop();

    void enqueue(void const* owner, ServerAction const& action);

private:
    detail::GMainContextHandle const main_context;
    std::atomic<bool> running;
};

}

#endif
