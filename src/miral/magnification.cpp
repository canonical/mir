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

#include "miral/magnification.h"
#include <mir/server.h>

#include "mir/shell/accessibility_manager.h"
#include "mir/shell/magnification_manager.h"

class miral::Magnification::Self
{
public:
    explicit Self(bool enabled_by_default) : enabled_by_default(enabled_by_default)
    {
    }

    bool enabled_by_default;
    float default_magnification = 2.0f;
    mir::geometry::Size default_size{400, 300};
    std::function<void()> on_enabled = []{};
    std::function<void()> on_disabled = []{};
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::Magnification::Magnification(bool enabled_by_default)
    : self(std::make_shared<Self>(enabled_by_default))
{
}

bool miral::Magnification::enabled(bool enabled) const
{
    if (auto const locked = self->accessibility_manager.lock())
    {
        if (locked->magnification_manager()->enabled(enabled))
        {
            if (!enabled)
                self->on_disabled();
            else
                self->on_enabled();
            return true;
        }
        return false;
    }

    self->enabled_by_default = enabled;
    return true;
}

void miral::Magnification::magnification(float magnification) const
{
    if (auto const locked = self->accessibility_manager.lock())
        locked->magnification_manager()->magnification(magnification);
    else
        self->default_magnification = magnification;
}

float miral::Magnification::magnification() const
{
    if (auto const locked = self->accessibility_manager.lock())
        return locked->magnification_manager()->magnification();
    else
        return self->default_magnification;
}

void miral::Magnification::size(mir::geometry::Size const& size) const
{
    if (auto const locked = self->accessibility_manager.lock())
        locked->magnification_manager()->size(size);
    else
        self->default_size = size;
}

mir::geometry::Size miral::Magnification::size() const
{
    if (auto const locked = self->accessibility_manager.lock())
        return locked->magnification_manager()->size();
    else
        return self->default_size;
}

miral::Magnification& miral::Magnification::on_enabled(std::function<void()>&& callback)
{
    self->on_enabled = std::move(callback);
    return *this;
}

miral::Magnification& miral::Magnification::on_disabled(std::function<void()>&& callback)
{
    self->on_disabled = std::move(callback);
    return *this;
}

void miral::Magnification::operator()(mir::Server& server) const
{
    server.add_init_callback([this, self=self, &server]
    {
        self->accessibility_manager = server.the_accessibility_manager();
        enabled(self->enabled_by_default);
        magnification(self->default_magnification);
        size(self->default_size);
    });
}