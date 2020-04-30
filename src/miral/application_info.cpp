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

#include "miral/application_info.h"
#include "miral/window.h"

#include <mir/scene/session.h>

struct miral::ApplicationInfo::Self
{
    Self() = default;
    explicit Self(Application const& app) : app{app} {}

    Application app;
    std::vector<Window> windows;
    std::shared_ptr<void> userdata;
};

miral::ApplicationInfo::ApplicationInfo() :
    self{std::make_unique<Self>()}
{
}

miral::ApplicationInfo::ApplicationInfo(Application const& app) :
    self{std::make_unique<Self>(app)}
{
}

miral::ApplicationInfo::~ApplicationInfo() = default;

miral::ApplicationInfo::ApplicationInfo(ApplicationInfo const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

auto miral::ApplicationInfo::operator=(ApplicationInfo const& that) -> ApplicationInfo&
{
    *self = *that.self;
    return *this;
}

auto miral::ApplicationInfo::name() const -> std::string
{
    return self->app ? self->app->name() : std::string{};
}

auto miral::ApplicationInfo::application() const -> Application
{
    return self->app;
}

auto miral::ApplicationInfo::windows() const -> std::vector <Window>&
{
    return self->windows;
}

auto miral::ApplicationInfo::userdata() const -> std::shared_ptr<void>
{
    return self->userdata;
}

void miral::ApplicationInfo::userdata(std::shared_ptr<void> userdata)
{
    self->userdata = userdata;
}
