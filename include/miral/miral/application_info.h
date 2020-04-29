/*
 * Copyright © 2015-2020 Canonical Ltd.
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

#ifndef MIRAL_APPLICATION_INFO_H
#define MIRAL_APPLICATION_INFO_H

#include "miral/application.h"

#include <string>
#include <vector>

namespace miral
{
class Window;

struct ApplicationInfo
{
    ApplicationInfo();
    explicit ApplicationInfo(Application const& app);
    ~ApplicationInfo();
    ApplicationInfo(ApplicationInfo const& that);
    auto operator=(ApplicationInfo const& that) -> miral::ApplicationInfo&;

    auto name() const -> std::string;
    auto application()  const -> Application;
    auto windows() const -> std::vector <Window>&;

    /// This can be used by client code to store window manager specific information
    auto userdata() const -> std::shared_ptr<void>;
    void userdata(std::shared_ptr<void> userdata);

private:
    friend class BasicWindowManager;
    void add_window(Window const& window);
    void remove_window(Window const& window);

    struct Self;
    std::unique_ptr<Self> const self;
};
}

#endif //MIRAL_APPLICATION_INFO_H
