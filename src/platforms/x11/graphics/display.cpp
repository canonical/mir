/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/c_memory.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_configuration.h"
#include <mir/graphics/display_configuration_policy.h>
#include "mir/graphics/egl_error.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/gl_config.h"
#include "display_configuration.h"
#include "display.h"
#include "platform.h"
#include "display_buffer.h"

#include <boost/throw_exception.hpp>
#include <algorithm>

#define MIR_LOG_COMPONENT "display"
#include "mir/log.h"

namespace mx=mir::X;
namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

namespace
{
auto get_pixel_size_mm(mx::XCBConnection* conn) -> geom::SizeF
{
    auto const screen = conn->screen();
    return geom::SizeF{
        float(screen->width_in_millimeters) / screen->width_in_pixels,
        float(screen->height_in_millimeters) / screen->height_in_pixels};
}
}

mgx::X11Window::X11Window(mx::X11Resources* x11_resources,
                          std::string title,
                          geom::Size const size)
    : x11_resources{x11_resources}
{
    auto const conn = x11_resources->conn.get();

    uint32_t const value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[32];
    value_list[0] = 0; // background_pixel
    value_list[1] = 0; // border_pixel
    value_list[2] = XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_EXPOSURE         |
                    XCB_EVENT_MASK_KEY_PRESS        |
                    XCB_EVENT_MASK_KEY_RELEASE      |
                    XCB_EVENT_MASK_BUTTON_PRESS     |
                    XCB_EVENT_MASK_BUTTON_RELEASE   |
                    XCB_EVENT_MASK_FOCUS_CHANGE     |
                    XCB_EVENT_MASK_ENTER_WINDOW     |
                    XCB_EVENT_MASK_LEAVE_WINDOW     |
                    XCB_EVENT_MASK_POINTER_MOTION;

    win = conn->generate_id();
    conn->create_window(win, size.width.as_int(), size.height.as_int(), value_mask, value_list);

    // Enable the WM_DELETE_WINDOW protocol for the window (causes a client message to be sent when window is closed)
    conn->change_property(win, x11_resources->WM_PROTOCOLS, XCB_ATOM_ATOM, 32, 1, &x11_resources->WM_DELETE_WINDOW);

    // Include hostname in title when X-forwarding
    if (getenv("SSH_CONNECTION"))
    {
        char buffer[128] = { '\0' };
        if (gethostname(buffer, sizeof buffer - 1) == 0)
        {
            title += " - ";
            title += buffer;
        }
    }

    conn->change_property(win, x11_resources->_NET_WM_NAME, x11_resources->UTF8_STRING, 8, title.size(), title.c_str());

    conn->map_window(win);
}

mgx::X11Window::~X11Window()
{
    x11_resources->conn->destroy_window(win);
}

mgx::X11Window::operator xcb_window_t() const
{
    return win;
}

mgx::Display::Display(
    std::shared_ptr<Platform> parent,
    std::shared_ptr<mir::X::X11Resources> const& x11_resources,
    std::string const title,
    std::vector<X11OutputConfig> const& requested_sizes,
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<DisplayReport> const& report)
    : parent{std::move(parent)},
      x11_resources{x11_resources},
      pixel_size_mm{get_pixel_size_mm(x11_resources->conn.get())},
      report{report}
{
    geom::Point top_left{0, 0};

    for (auto const& requested_size : requested_sizes)
    {
        auto actual_size = requested_size.size;
        auto window = std::make_unique<X11Window>(
            x11_resources.get(),
            title,
            actual_size);
        auto pf = x11_resources->conn->default_pixel_format();
        auto configuration = DisplayConfiguration::build_output(
            pf,
            actual_size,
            top_left,
            geom::Size{
                actual_size.width * pixel_size_mm.width.as_value(),
                actual_size.height * pixel_size_mm.height.as_value()},
            requested_size.scale,
            mir_orientation_normal);
        auto display_buffer = std::make_unique<mgx::DisplayBuffer>(
            this->parent,
            *window,
            configuration->extents());
        top_left.x += as_delta(configuration->extents().size.width);
        outputs.push_back(std::make_unique<OutputInfo>(
            this,
             std::move(window),
             std::move(display_buffer),
             std::move(configuration)));
    }

    auto const display_config = configuration();
    initial_conf_policy->apply_to(*display_config);
    configure(*display_config);
    report->report_successful_display_construction();
    x11_resources->conn->flush();
}

mgx::Display::~Display() noexcept
{
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
    std::lock_guard lock{mutex};
    for (auto const& output : outputs)
    {
        f(*output->display_buffer);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
    std::lock_guard lock{mutex};
    std::vector<DisplayConfigurationOutput> output_configurations;
    for (auto const& output : outputs)
    {
        output_configurations.push_back(*output->config);
    }
    return std::make_unique<mgx::DisplayConfiguration>(output_configurations);
}

void mgx::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    std::lock_guard lock{mutex};

    if (!new_configuration.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

    new_configuration.for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        bool found_info = false;

        for (auto& output : outputs)
        {
            if (output->config->id == conf_output.id)
            {
                *output->config = conf_output;
                output->display_buffer->set_view_area(output->config->extents());
                switch (output->config->power_mode)
                {
                case mir_power_mode_on:
                    output->display_buffer->set_transformation(output->config->transformation());
                    break;

                case mir_power_mode_standby:
                case mir_power_mode_suspend:
                case mir_power_mode_off:
                    // Simulate an off display by setting a zeroed-out transform
                    output->display_buffer->set_transformation(glm::mat2{0});
                    break;
                }
                found_info = true;
                break;
            }
        }

        if (!found_info)
            mir::log_error("Could not find info for output %d", conf_output.id.as_value());
    });
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler*/,
    DisplayConfigurationChangeHandler const& change_handler)
{
    std::lock_guard lock{mutex};
    config_change_handlers.push_back(change_handler);
}

void mgx::Display::pause()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on x11 platform"));
}

void mgx::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on x11 platform"));
}

auto mgx::Display::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return nullptr;
}

bool mgx::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& /*conf*/)
{
    return false;
}

mgx::Display::OutputInfo::OutputInfo(
    Display* owner,
    std::unique_ptr<X11Window> window,
    std::unique_ptr<DisplayBuffer> display_buffer,
    std::shared_ptr<DisplayConfigurationOutput> configuration)
    : owner{owner},
      window{std::move(window)},
      display_buffer{std::move(display_buffer)},
      config{std::move(configuration)}
{
    owner->x11_resources->set_set_output_for_window(*this->window, this);
}

mgx::Display::OutputInfo::~OutputInfo()
{
    owner->x11_resources->clear_output_for_window(*window);
}

void mgx::Display::OutputInfo::set_size(geometry::Size const& size)
{
    std::unique_lock lock{owner->mutex};
    if (config->modes[0].size == size)
    {
        return;
    }
    config->modes[0].size = size;
    display_buffer->set_view_area(config->extents());
    auto const handlers = owner->config_change_handlers;

    lock.unlock();
    for (auto const& handler : handlers)
    {
        handler();
    }
}
