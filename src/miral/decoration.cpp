/*
 * Copyright © Canonical Ltd.
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
 */

#include "miral/decoration.h"
#include "mir/shell/decoration/input_resolver.h"
#include <memory>

struct miral::DeviceEvent::Impl
{
    mir::shell::decoration::DeviceEvent event;
};

miral::DeviceEvent::DeviceEvent(mir::shell::decoration::DeviceEvent event)
    : impl{std::make_unique<Impl>(event)}
{
}

miral::DeviceEvent::operator mir::shell::decoration::DeviceEvent() const
{
    return impl->event;
}

auto miral::DeviceEvent::location() const -> mir::geometry::Point
{
    return impl->event.location;
}

auto miral::DeviceEvent::button_down(MirPointerButton button) const -> bool {
    return impl->event.mouse_buttons_state->button_down(button);
}

void miral::DecorationRedrawNotifier::notify() const
{
    if (on_redraw)
        on_redraw();
}

void miral::DecorationRedrawNotifier::register_listener(std::function<void()> on_redraw)
{
    this->on_redraw = on_redraw;
}

void miral::DecorationRedrawNotifier::clear_listener()
{
    this->on_redraw = {};
}

struct miral::Decoration::Self
{
    Self();
    std::shared_ptr<DecorationRedrawNotifier> redraw_notifier;
};

miral::Decoration::Self::Self() :
    redraw_notifier{std::make_shared<DecorationRedrawNotifier>()}
{
}

miral::Decoration::Decoration()
    : self{std::make_shared<Self>()}
{
}

std::shared_ptr<miral::DecorationRedrawNotifier> miral::Decoration::redraw_notifier()
{
    return self->redraw_notifier;
}
