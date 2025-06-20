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

#ifndef MIR_TESTWLCSDISPLAYSERVER_H
#define MIR_TESTWLCSDISPLAYSERVER_H

#include <miral/test_display_server.h>
#include <wlcs/display_server.h>

#include <atomic>

namespace mir
{
class Executor;
namespace test  { class Signal; }
}

namespace miral
{
struct TestWlcsDisplayServer : TestDisplayServer, public WlcsDisplayServer
{
    TestWlcsDisplayServer(int argc, char const** argv);

    void start_server();

    int create_client_socket();

    void position_window(wl_display* client, wl_surface* surface, mir::geometry::Point point);

    WlcsPointer* create_pointer();

    WlcsTouch* create_touch();

    std::shared_ptr<mir::test::Signal> expect_event_with_time(std::chrono::nanoseconds event_time);

    struct FakePointer;
private:
    class InputEventListener;
    class ResourceMapper;
    class RunnerCursorObserver;

    std::shared_ptr<ResourceMapper> const resource_mapper;
    std::shared_ptr<InputEventListener> const event_listener;
    std::shared_ptr<mir::Executor> executor;
    std::atomic<double> cursor_x{0}, cursor_y{0};
    std::shared_ptr<RunnerCursorObserver> cursor_observer;

    mir::Server* mir_server = nullptr;
};
}

#endif //MIR_TESTWLCSDISPLAYSERVER_H
