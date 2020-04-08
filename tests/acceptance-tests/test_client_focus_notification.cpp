/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/connected_client_headless_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace ::testing;

namespace
{
struct FocusSurface
{
    FocusSurface(MirConnection* connection) :
        connection(connection)
    {
        surface = mir_connection_create_render_surface_sync(connection, 100, 100);
        auto spec = mir_create_normal_window_spec(connection, 100, 100);
        mir_window_spec_set_event_handler(spec, FocusSurface::handle_event, this);
        mir_window_spec_add_render_surface(spec, surface, 100, 100, 0, 0);
        window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);

        mir_buffer_stream_swap_buffers_sync(
            mir_render_surface_get_buffer_stream(surface, 100, 100, mir_pixel_format_argb_8888));
    }

    ~FocusSurface()
    {
        if (!released) release();
    }

    static void handle_event(MirWindow* window, MirEvent const* ev, void* context)
    {
        if (mir_event_type_window == mir_event_get_type(ev))
        {
            auto surface_ev = mir_event_get_window_event(ev);
            auto attrib = mir_window_event_get_attribute(surface_ev);
            if (mir_window_attrib_focus == attrib)
            {
                auto self = static_cast<FocusSurface*>(context);
                self->log_focus_event(window,
                    static_cast<MirWindowFocusState>(mir_window_event_get_attribute_value(surface_ev)));
            }
        }
    }

    void log_focus_event(MirWindow*, MirWindowFocusState state)
    {
        std::lock_guard<std::mutex> lk(mutex);
        focus_events.push_back(state);
        cv.notify_all();
    }

    MirWindow* native_handle() const
    {
        return window;
    }

    void release()
    {
        mir_window_release_sync(window);
        mir_connection_release(connection);
        released = true;
    }

    void expect_focus_event_sequence(std::vector<MirWindowFocusState> const& seq)
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, timeout, [this, &seq]
            {
                return focus_events.size() >= seq.size();
            }))
        {
            throw std::logic_error("timeout waiting for events");
        }
        EXPECT_THAT(focus_events, ContainerEq(seq));
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<MirWindowFocusState> focus_events;
    bool released {false};
    MirConnection* connection = nullptr;
    MirRenderSurface* surface = nullptr;
    MirWindow* window = nullptr;
    std::chrono::seconds timeout{5};
};

struct ClientFocusNotification : mtf::ConnectedClientHeadlessServer
{
    std::unique_ptr<FocusSurface> make_surface()
    {
        auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
        EXPECT_THAT(connection, NotNull());
        return std::make_unique<FocusSurface>(connection);
    }
};
}

TEST_F(ClientFocusNotification, a_surface_is_notified_of_receiving_focus)
{
    std::vector<MirWindowFocusState> const focus_sequence = {mir_window_focus_state_focused};
    auto window = make_surface();
    window->expect_focus_event_sequence(focus_sequence);
    window->release();
}

TEST_F(ClientFocusNotification, two_surfaces_are_notified_of_gaining_and_losing_focus)
{
    std::vector<MirWindowFocusState> const initial_focus = {mir_window_focus_state_focused};
    std::vector<MirWindowFocusState> const focus_sequence1 =
        {mir_window_focus_state_focused, mir_window_focus_state_unfocused, mir_window_focus_state_focused};
    std::vector<MirWindowFocusState> const focus_sequence2 = {mir_window_focus_state_focused};

    auto surface1 = make_surface();
    surface1->expect_focus_event_sequence(initial_focus);

    auto surface2 = make_surface();
    surface2->expect_focus_event_sequence(focus_sequence2);
    surface2->release();

    surface1->expect_focus_event_sequence(focus_sequence1);
    surface1->release();
}
