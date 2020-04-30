/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_APPLICATION_INFO_INTERNAL_H
#define MIR_APPLICATION_INFO_INTERNAL_H

#include "miral/application_info.h"
#include "miral/window.h"

struct miral::ApplicationInfo::Self
{
    Self() = default;
    explicit Self(Application const& app) : app{app} {}

    Application app;
    std::vector<Window> windows;
    std::shared_ptr<void> userdata;
};

#endif  // MIR_APPLICATION_INFO_INTERNAL_H
