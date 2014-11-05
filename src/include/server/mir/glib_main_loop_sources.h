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

#ifndef MIR_GLIB_MAIN_LOOP_SOURCES_H_
#define MIR_GLIB_MAIN_LOOP_SOURCES_H_

#include <functional>
#include <vector>
#include <mutex>
#include <memory>

#include <glib.h>

namespace mir
{
namespace detail
{

class GSourceHandle
{
public:
    explicit GSourceHandle(GSource* gsource);
    GSourceHandle(GSourceHandle&& other);
    ~GSourceHandle();

    void attach(GMainContext* main_context) const;
    void attach_and_detach_on_destruction(GMainContext* main_context);

private:
    GSource* gsource;
    bool detach_on_destruction;
};

GSourceHandle make_idle_gsource(int priority, std::function<void()> const& callback);
GSourceHandle make_signal_gsource(int sig, std::function<void(int)> const& callback);

class FdSources
{
public:
    FdSources(GMainContext* main_context);
    ~FdSources();

    void add(int fd, void const* owner, std::function<void(int)> const& handler);
    void remove_all_owned_by(void const* owner);

private:
    struct FdContext;
    struct FdSource;

    GMainContext* const main_context;
    std::mutex sources_mutex;
    std::vector<std::unique_ptr<FdSource>> sources;
};

}
}

#endif
