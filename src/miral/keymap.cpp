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

#include "miral/keymap.h"

#include "gdbus.h"
#include "miral/live_config.h"

#include <mir/fd.h>
#include <mir/input/input_device_observer.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/device.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <mir/udev/wrapper.h>

#include <mir/input/parameter_keymap.h>
#include <mir/input/mir_keyboard_config.h>

#define MIR_LOG_COMPONENT "miral::Keymap"
#include <mir/log.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#include <boost/program_options/options_description.hpp>

namespace mi = mir::input;

namespace
{
auto constexpr keymap_description = "keymap <layout>[+<variant>[+<options>]], e,g, \"gb\" or \"cz+qwerty\" or \"de++compose:caps\"";

std::string keymap_default()
{
    static auto const default_keymap = "us";

    namespace mu = mir::udev;

    mu::Enumerator input_enumerator{std::make_shared<mu::Context>()};
    input_enumerator.match_subsystem("input");
    input_enumerator.scan_devices();

    for (auto& device : input_enumerator)
    {
        if (auto const layout  = device.property("XKBLAYOUT"))
        {
            auto const options = device.property("XKBOPTIONS");
            auto const variant = device.property("XKBVARIANT");

            std::string keymap{layout};

            if (variant || options)
            {
                keymap += '+';

                if (variant)
                {
                    keymap += variant;
                }

                if (options)
                {
                    keymap += '+';
                    keymap += options;
                }
            }

            return keymap;
        }
    }

    return default_keymap;
}

char const* const keymap_option = "keymap";
}

struct miral::Keymap::Self : mir::input::InputDeviceObserver
{
    Self(std::string_view keymap) : layout{}, variant{}
    {
        set_keymap(keymap);
    }

    void set_keymap(std::string_view keymap)
    {
        std::lock_guard lock{mutex};
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
    try
    {
        std::lock_guard lock{mutex};

        if (mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard))
            add_keyboard(device);
    }
    catch (...)
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(),
                 "problem adding device (" + device->name() + ")");
    }

    void device_changed(std::shared_ptr<mir::input::Device> const& device) override
    {
        std::lock_guard lock{mutex};

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
        std::string model = mi::ParameterKeymap::default_model;
        auto const keyboard_config = keyboard->keyboard_configuration();
        if (keyboard_config.is_set())
        {
            model = keyboard_config.value().device_keymap()->model();
        }
        std::shared_ptr<mi::Keymap> keymap{std::make_shared<mi::ParameterKeymap>(model, layout, variant, options)};
        keyboard->apply_keyboard_configuration(std::move(keymap));
    }

    void device_removed(std::shared_ptr<mir::input::Device> const& device) override
    try
    {
        std::lock_guard lock{mutex};

        if (mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard))
            keyboards.erase(std::find(begin(keyboards), end(keyboards), device));
    }
    catch (...)
    {
        mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT, std::current_exception(),
            "problem removing device (" + device->name() + ")");
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

miral::Keymap::Keymap(live_config::Store& config_store) : Keymap{keymap_default()}
{
    config_store.add_string_attribute(
        {keymap_option},
        keymap_description,
        keymap_default(),
        [self=self](live_config::Key const&, std::optional<std::string_view> val)
            {
                if (val)
                {
                    self->set_keymap(*val);
                }
            });
}

miral::Keymap::~Keymap() = default;

miral::Keymap::Keymap(Keymap const&) = default;

auto miral::Keymap::operator=(Keymap const& rhs) -> Keymap& = default;

void miral::Keymap::operator()(mir::Server& server) const
{
    if (self->layout.empty())
    {
        server.add_configuration_option(keymap_option, keymap_description, keymap_default());
    }

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

miral::Keymap::Keymap(std::unique_ptr<Self>&& self) :
    self{std::move(self)}
{
}

namespace
{
using namespace miral::gdbus;

char const* const interface_name = "org.freedesktop.locale1";
char const* const bus_name = interface_name;
char const* const sender = interface_name;
char const* const object_path = "/org/freedesktop/locale1";
char const* const properties_interface = "org.freedesktop.DBus.Properties";
char const* const signal_name = "PropertiesChanged";

auto read_entry(Connection const& connection, char const* entry) -> std::optional<std::string>
{
    static char const* const method_name = "Get";

    g_autoptr(GError) error = nullptr;

    if (Variant const result{g_dbus_connection_call_sync(connection,
                                                        bus_name,
                                                        object_path,
                                                        properties_interface,
                                                        method_name,
                                                        g_variant_new("(ss)", interface_name, entry),
                                                        G_VARIANT_TYPE("(v)"),
                                                        G_DBUS_CALL_FLAGS_NONE,
                                                        G_MAXINT,
                                                        nullptr,
                                                        &error)})
    {
        Variant const unwrap{g_variant_get_child_value(result, 0)};
        Variant const unwrap2{g_variant_get_child_value(unwrap, 0)};

        if (g_variant_is_of_type(unwrap2, G_VARIANT_TYPE_STRING))
        {
            return g_variant_get_string(unwrap2, nullptr);
        }
    }

    if (error)
    {
        mir::log_info("Dbus error=%s, dest=%s, object_path=%s, properties_interface=%s, method_name=%s, interface_name=%s",
                      error->message, bus_name, object_path, properties_interface, method_name, interface_name);
    }

    return std::nullopt;
}

auto read_keymap(Connection const& connection) -> std::optional<std::string>
{
    if (auto const layout = read_entry(connection, "X11Layout"))
    {
        auto const variant = read_entry(connection, "X11Variant");
        auto const options = read_entry(connection, "X11Options");

        return *layout + '+' + variant.value_or("") + '+' + options.value_or("");
    }

    return std::nullopt;
}

template<typename T>
void callback_thunk(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant *parameters, gpointer self)
{
    auto const self_ptr = static_cast<T*>(self);
    self_ptr->callback(parameters);
}
}

auto miral::Keymap::system_locale1() -> Keymap
{
    struct SystemLocalSelf : Self
    {
        explicit SystemLocalSelf(Connection const&& connection) :
            Self{read_keymap(connection).value_or("us")},
            watch_id{g_dbus_connection_signal_subscribe(
                connection,
                sender,
                properties_interface,
                signal_name,
                object_path,
                nullptr,
                G_DBUS_SIGNAL_FLAGS_NONE,
                callback_thunk<SystemLocalSelf>,
                this,
                nullptr)},
            connection{std::move(connection)},
            main_loop{MainLoop::the_main_loop()}
        {
        }

        ~SystemLocalSelf() override
        {
            g_dbus_connection_signal_unsubscribe(connection, watch_id);
        }

        guint watch_id = 0;
        Connection const connection;
        std::shared_ptr<MainLoop> const main_loop;

        void callback(GVariant* parameters)
        {
            std::optional<std::string> layout;
            std::optional<std::string> options;
            std::optional<std::string> variant;

            g_autoptr(GVariantIter) changed_properties = nullptr;
            g_variant_get(parameters, "(sa{sv}as)",
                         nullptr,
                         &changed_properties,
                         nullptr);

            const char* key;
            GVariant* value;
            while (g_variant_iter_loop(changed_properties, "{&sv}", &key, &value))
            {
                if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
                {
                    using namespace std::string_literals;

                    if (key == "X11Layout"s)
                    {
                        layout = g_variant_get_string(value, nullptr);
                    }

                    if (key == "X11Variant"s)
                    {
                        variant = g_variant_get_string(value, nullptr);
                    }

                    if (key == "X11Options"s)
                    {
                        options = g_variant_get_string(value, nullptr);
                    }
                }
            }

            if (layout)
            {
                set_keymap(*layout + '+' + variant.value_or("") + '+' + options.value_or(""));
            }
        }
    };

    return Keymap{std::make_unique<SystemLocalSelf>(Connection{g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr)})};
}
