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

#include <miroil/mir_server_hooks.h>

#include "gesture_ender.h"

// mir
#include <mir/server.h>
#include <mir/graphics/cursor.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_observer.h>
#include <mir/input/cursor_images.h>
#include <mir/version.h>

#include <memory>

namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace
{
struct MirInputDeviceObserverImpl : mir::input::InputDeviceObserver
{
public:
    MirInputDeviceObserverImpl(std::shared_ptr<miroil::InputDeviceObserver> & observer) : observer(observer) {};

    void device_added(std::shared_ptr<mir::input::Device> const& device) override;
    void device_changed(std::shared_ptr<mir::input::Device> const& /*device*/) override {}
    void device_removed(std::shared_ptr<mir::input::Device> const& device) override;
    void changes_complete() override {}

private:
    std::shared_ptr<miroil::InputDeviceObserver> observer;
};

struct HiddenCursorWrapper : mg::Cursor
{
    HiddenCursorWrapper(std::shared_ptr<mg::Cursor> const& wrapped) :
        wrapped{wrapped} { wrapped->hide(); }
    void show(std::shared_ptr<mg::CursorImage> const&) override { }
    void hide() override { wrapped->hide(); }

    void move_to(mir::geometry::Point position) override { wrapped->move_to(position); }

    void scale(float new_scale) override { wrapped->scale(new_scale); }

    auto renderable() -> std::shared_ptr<mir::graphics::Renderable> override { return wrapped->renderable(); }

    auto needs_compositing() const -> bool override { return wrapped->needs_compositing(); }

private:
    std::shared_ptr<mg::Cursor> const wrapped;
};
}

class MirCursorImages : public mir::input::CursorImages
{
public:
    MirCursorImages(miroil::CreateNamedCursor func);

    std::shared_ptr<mir::graphics::CursorImage> image(const std::string &cursor_name,
            const mir::geometry::Size &size) override;

private:
    miroil::CreateNamedCursor create_func;
};

MirCursorImages::MirCursorImages(miroil::CreateNamedCursor func)
{
    create_func = func;
}

auto MirCursorImages::image(const std::string &cursor_name, const mir::geometry::Size &)
-> std::shared_ptr<mir::graphics::CursorImage>
{
    return create_func(cursor_name);
}

struct miroil::MirServerHooks::Self
{
    std::weak_ptr<mir::graphics::Display> mir_display;
    std::weak_ptr<mir::shell::DisplayConfigurationController> mir_display_configuration_controller;
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub;
    CreateNamedCursor create_cursor;
};

miroil::MirServerHooks::MirServerHooks() :
    self{std::make_shared<Self>()}
{
}

void miroil::MirServerHooks::operator()(mir::Server& server)
{
    server.override_the_pointer_input_dispatcher([] { return the_pointer_input_dispatcher(); });

    if (self->create_cursor) {
        server.override_the_cursor_images([this]
            { return std::make_shared<MirCursorImages>(self->create_cursor); });
    }

    server.wrap_cursor([&](std::shared_ptr<mg::Cursor> const& wrapped)
        { return std::make_shared<HiddenCursorWrapper>(wrapped); });

    server.add_init_callback([this, &server]
        {
            self->mir_display = server.the_display();
            self->mir_display_configuration_controller = server.the_display_configuration_controller();
            self->input_device_hub = server.the_input_device_hub();
        });
}

auto miroil::MirServerHooks::the_mir_display() const
-> std::shared_ptr<mir::graphics::Display>
{
    if (auto result = self->mir_display.lock())
        return result;

    throw std::logic_error("No display available. Server not running?");
}

auto miroil::MirServerHooks::the_display_configuration_controller() const
-> std::shared_ptr<mir::shell::DisplayConfigurationController>
{
    if (auto result = self->mir_display_configuration_controller.lock())
        return result;

    throw std::logic_error("No input device hub available. Server not running?");
}

void miroil::MirServerHooks::create_named_cursor(CreateNamedCursor func)
{
    self->create_cursor = func;
}

void miroil::MirServerHooks::create_input_device_observer(std::shared_ptr<miroil::InputDeviceObserver> & observer)
{
    if (auto result = self->input_device_hub.lock()) {
        result->add_observer(std::make_shared<MirInputDeviceObserverImpl>(observer));
	return;
    }

    throw std::logic_error("No input device hub available. Server not running?");
}

void MirInputDeviceObserverImpl::device_added(const std::shared_ptr<mir::input::Device> &device)
{
    observer->device_added(miroil::InputDevice(device));
}

void MirInputDeviceObserverImpl::device_removed(const std::shared_ptr<mir::input::Device> &device)
{
    if (device) {
        observer->device_removed(miroil::InputDevice(device));
    }
}
