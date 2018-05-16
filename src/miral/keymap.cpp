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

#include "miral/keymap.h"

#include <mir/input/input_device_observer.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/device.h>
#include <mir/options/option.h>
#include <mir/server.h>

#include <mir/input/keymap.h>
#include <mir/input/mir_keyboard_config.h>

#define MIR_LOG_COMPONENT "miral::Keymap"
#include <mir/log.h>

#include <cstdio>
#include <memory>
#include <string>

#include <algorithm>
#include <mutex>
#include <vector>

namespace
{
std::string keymap_default()
{
    static auto const default_keymap = "us";
    static char const locale_text[] = "Layout:";

    std::unique_ptr<FILE, void(*)(FILE*)> localectl{popen("localectl status", "r"), [](FILE* f){ pclose(f);}};

    if (!localectl)
        return default_keymap;

    char buf[1024];
    std::string line;

    while (auto got = fgets(buf, sizeof buf, localectl.get()))
    {
        line.append(got);

        if (got && !feof(localectl.get()) && !line.empty() && line.back() != '\n')
            continue;

        if (!line.empty() && line.back() == '\n')
            line.pop_back();

        auto const locale_pos = line.find(locale_text);
        if (locale_pos != std::string::npos)
            return line.substr(locale_pos + sizeof locale_text);

        line.clear();
    }

    return default_keymap;
}

char const* const keymap_option = "keymap";
}

struct miral::Keymap::Self : mir::input::InputDeviceObserver
{
    Self(std::string const& keymap) : layout{}, variant{}
    {
        set_keymap(keymap);
    }

    void set_keymap(std::string const& keymap)
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        auto get_next_token = [km = keymap]() mutable
        {
            auto const i = km.find('+');
            auto ret = km.substr(0,i);
            if (i != std::string::npos)
                km = km.substr(i+1, std::string::npos);
            else
                km = "";
            return ret;
        };

        layout = get_next_token();
        variant = get_next_token();
        options = get_next_token();

        for (auto const& keyboard : keyboards)
            apply_keymap(keyboard);
    }

    void device_added(std::shared_ptr<mir::input::Device> const& device) override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};

        if (mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard))
            add_keyboard(device);
    }

    void device_changed(std::shared_ptr<mir::input::Device> const& device) override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};

        auto const keyboard = std::find(begin(keyboards), end(keyboards), device);

        if (mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard))
        {
            if (keyboard == end(keyboards))
                add_keyboard(device);
        }
        else
        {
            if (keyboard != end(keyboards))
                keyboards.erase(keyboard);
        }
    }

    void add_keyboard(std::shared_ptr<mir::input::Device> const& keyboard)
    {
        keyboards.push_back(keyboard);
        apply_keymap(keyboard);
    }

    void apply_keymap(std::shared_ptr<mir::input::Device> const& keyboard)
    {
        auto const keyboard_config = keyboard->keyboard_configuration();
        mir::input::Keymap keymap;

        if (keyboard_config.is_set())
        {
            keymap = keyboard_config.value().device_keymap();
        }

        keymap.layout = layout;
        keymap.variant = variant;
        keymap.options = options;
        keyboard->apply_keyboard_configuration(std::move(keymap));
    }

    void device_removed(std::shared_ptr<mir::input::Device> const& device) override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};

        if (mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard))
            keyboards.erase(std::find(begin(keyboards), end(keyboards), device));
    }

    void changes_complete() override
    {
    }

    std::mutex mutable mutex;
    std::string layout;
    std::string variant;
    std::string options;
    std::vector<std::shared_ptr<mir::input::Device>> keyboards;
};

miral::Keymap::Keymap() :
    self{std::make_shared<Self>(std::string{})}
{
}

miral::Keymap::Keymap(std::string const& keymap) :
    self{std::make_shared<Self>(keymap)}
{
}

miral::Keymap::~Keymap() = default;

miral::Keymap::Keymap(Keymap const&) = default;

auto miral::Keymap::operator=(Keymap const& rhs) -> Keymap& = default;

void miral::Keymap::operator()(mir::Server& server) const
{
    if (self->layout.empty())
        server.add_configuration_option(keymap_option, "keymap <layout>[+<variant>[+<options>]], e,g, \"gb\" or \"cz+qwerty\" or \"de++compose:caps\"", keymap_default());

    server.add_init_callback([this, &server]
        {
            if (self->layout.empty())
                self->set_keymap(server.get_options()->get<std::string>(keymap_option));

            server.the_input_device_hub()->add_observer(self);
        });
}

void miral::Keymap::set_keymap(std::string const& keymap)
{
    self->set_keymap(keymap);
}
