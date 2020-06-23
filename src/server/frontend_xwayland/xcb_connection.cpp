/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 */

#include "xcb_connection.h"

#include "mir/log.h"

#include "boost/throw_exception.hpp"
#include <sstream>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace
{
template<typename T, typename U = T>
auto data_buffer_to_debug_string(
    T* data,
    size_t elements,
    std::function<U(T element)> converter = [](T e) -> T { return e; }) -> std::string
{
    std::stringstream ss;
    ss << "[";
    for (T* i = data; i != data + elements; i++)
    {
        if (i != data)
            ss << ", ";
        ss << converter(*i);
    }
    ss << "]";
    return ss.str();
}

template<typename T, size_t length>
constexpr size_t length_of(T(&)[length])
{
    return length;
}

/// Convert any sort of ID into a human-like readable (and memorable) name for debugging
/// 0 results in Null, and IDs that are close together often rhyme
auto number_to_readable_name(uint32_t number) -> std::string
{
    static const char* beginnings[]{"N", "Al", "Aver", "B", "Ch", "D", "Ell", "Ev", "F", "G", "Gr", "H", "Iv", "J", "K",
        "L", "M", "Mr ", "Ms ", "Mx ", "P", "Pr", "Qu", "R", "S", "T", "Tr", "V", "W", "Z"};
    static const char* middles[]{"u", "a", "ai", "ay", "e", "ee", "el", "en", "i", "ing", "o", "oo", "ou", "or"};
    static const char* ends[]{"ll", "b", "ck", "cy", "d", "dy", "gh", "gia", "r", "l", "le", "liet", "lie", "mer", "nd",
        "ny", "rd", "sly", "son", "ss", "thy", "te", "tt", "xon", "y", "yton", "zz", ""};
    std::stringstream ss;
    ss << beginnings[number % length_of(beginnings)];
    number /= length_of(beginnings);
    ss << middles[number % length_of(middles)];
    number /= length_of(middles);
    ss << ends[number % length_of(ends)];
    return ss.str();
}

auto xcb_error_to_string(int error) -> std::string
{
    // see https://xcb.freedesktop.org/manual/group__XCB__Core__API.html#ga70a6bade94bd2824db552abcf5fbdbe3
    switch (error)
    {
        case 0: return "no error";
        case XCB_CONN_ERROR: return "XCB_CONN_ERROR";
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED: return "XCB_CONN_CLOSED_EXT_NOTSUPPORTED";
        case XCB_CONN_CLOSED_MEM_INSUFFICIENT: return "XCB_CONN_CLOSED_MEM_INSUFFICIENT";
        case XCB_CONN_CLOSED_REQ_LEN_EXCEED: return "XCB_CONN_CLOSED_REQ_LEN_EXCEED";
        case XCB_CONN_CLOSED_PARSE_ERR: return "XCB_CONN_CLOSED_PARSE_ERR";
        case XCB_CONN_CLOSED_INVALID_SCREEN: return "XCB_CONN_CLOSED_INVALID_SCREEN";
        case XCB_CONN_CLOSED_FDPASSING_FAILED: return "XCB_CONN_CLOSED_FDPASSING_FAILED";
        default:;
    }
    return "unknwon XCB error " + std::to_string(error);
}

auto connect_to_fd(mir::Fd const& fd) -> xcb_connection_t*
{
    xcb_connection_t* connection = xcb_connect_to_fd(fd, nullptr);
    if (auto const error = xcb_connection_has_error(connection))
    {
        xcb_disconnect(connection);
        BOOST_THROW_EXCEPTION(std::runtime_error("xcb_connect_to_fd() failed: " + xcb_error_to_string(error)));
    }
    else
    {
        return connection;
    }
}
}

mf::XCBConnection::Atom::Atom(std::string const& name, XCBConnection* connection)
    : connection{connection},
      name_{name},
      cookie{xcb_intern_atom(*connection, 0, name_.size(), name_.c_str())}
{
}

mf::XCBConnection::Atom::operator xcb_atom_t() const
{
    if (atom == XCB_ATOM_NONE)
    {
        std::lock_guard<std::mutex> lock{mutex};

        // The initial atomic check is faster, but we need to check again once we've got the lock
        if (atom == XCB_ATOM_NONE)
        {
            auto const reply = xcb_intern_atom_reply(*connection, cookie, nullptr);
            if (!reply)
                BOOST_THROW_EXCEPTION(std::runtime_error("Failed to look up atom " + name_));
            atom = reply->atom;
            free(reply);

            std::lock_guard<std::mutex> lock{connection->atom_name_cache_mutex};
            connection->atom_name_cache[atom] = name_;
        }
    }

    return atom;
}

mf::XCBConnection::XCBConnection(Fd const& fd)
    : fd{fd},
      xcb_connection{connect_to_fd(fd)},
      xcb_screen{xcb_setup_roots_iterator(xcb_get_setup(xcb_connection)).data},
      atom_name_cache{{XCB_ATOM_NONE, "None/Any"}},
      wm_protocols{"WM_PROTOCOLS", this},
      wm_take_focus{"WM_TAKE_FOCUS", this},
      wm_delete_window{"WM_DELETE_WINDOW", this},
      wm_state{"WM_STATE", this},
      wm_change_state{"WM_CHANGE_STATE", this},
      wm_s0{"WM_S0", this},
      net_wm_cm_s0{"_NET_WM_CM_S0", this},
      net_wm_name{"_NET_WM_NAME", this},
      net_wm_pid{"_NET_WM_PID", this},
      net_wm_state{"_NET_WM_STATE", this},
      net_wm_state_maximized_vert{"_NET_WM_STATE_MAXIMIZED_VERT", this},
      net_wm_state_maximized_horz{"_NET_WM_STATE_MAXIMIZED_HORZ", this},
      net_wm_state_hidden{"_NET_WM_STATE_HIDDEN", this},
      net_wm_state_fullscreen{"_NET_WM_STATE_FULLSCREEN", this},
      net_wm_user_time{"_NET_WM_USER_TIME", this},
      net_wm_icon_name{"_NET_WM_ICON_NAME", this},
      net_wm_desktop{"_NET_WM_DESKTOP", this},
      net_wm_window_type{"_NET_WM_WINDOW_TYPE", this},
      net_wm_window_type_desktop{"_NET_WM_WINDOW_TYPE_DESKTOP", this},
      net_wm_window_type_dock{"_NET_WM_WINDOW_TYPE_DOCK", this},
      net_wm_window_type_toolbar{"_NET_WM_WINDOW_TYPE_TOOLBAR", this},
      net_wm_window_type_menu{"_NET_WM_WINDOW_TYPE_MENU", this},
      net_wm_window_type_utility{"_NET_WM_WINDOW_TYPE_UTILITY", this},
      net_wm_window_type_splash{"_NET_WM_WINDOW_TYPE_SPLASH", this},
      net_wm_window_type_dialog{"_NET_WM_WINDOW_TYPE_DIALOG", this},
      net_wm_window_type_dropdown{"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", this},
      net_wm_window_type_popup{"_NET_WM_WINDOW_TYPE_POPUP_MENU", this},
      net_wm_window_type_tooltip{"_NET_WM_WINDOW_TYPE_TOOLTIP", this},
      net_wm_window_type_notification{"_NET_WM_WINDOW_TYPE_NOTIFICATION", this},
      net_wm_window_type_combo{"_NET_WM_WINDOW_TYPE_COMBO", this},
      net_wm_window_type_dnd{"_NET_WM_WINDOW_TYPE_DND", this},
      net_wm_window_type_normal{"_NET_WM_WINDOW_TYPE_NORMAL", this},
      net_wm_moveresize{"_NET_WM_MOVERESIZE", this},
      net_supporting_wm_check{"_NET_SUPPORTING_WM_CHECK", this},
      net_supported{"_NET_SUPPORTED", this},
      net_active_window{"_NET_ACTIVE_WINDOW", this},
      motif_wm_hints{"_MOTIF_WM_HINTS", this},
      clipboard{"CLIPBOARD", this},
      clipboard_manager{"CLIPBOARD_MANAGER", this},
      utf8_string{"UTF8_STRING", this},
      wl_surface_id{"WL_SURFACE_ID", this}
{
}

mf::XCBConnection::~XCBConnection()
{
    xcb_disconnect(xcb_connection);
}

void mf::XCBConnection::verify_not_in_error_state() const
{
    if (auto const error = xcb_connection_has_error(xcb_connection))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("XCB connection shut down: " + xcb_error_to_string(error)));
    }
}

auto mf::XCBConnection::query_name(xcb_atom_t atom) const -> std::string
{
    std::lock_guard<std::mutex>{atom_name_cache_mutex};
    auto const iter = atom_name_cache.find(atom);

    if (iter == atom_name_cache.end())
    {
        xcb_get_atom_name_cookie_t const cookie = xcb_get_atom_name(xcb_connection, atom);
        xcb_get_atom_name_reply_t* const reply = xcb_get_atom_name_reply(xcb_connection, cookie, nullptr);

        std::string name;

        if (reply)
            name = std::string{xcb_get_atom_name_name(reply), static_cast<size_t>(xcb_get_atom_name_name_length(reply))};
        else
            name = "Atom " + std::to_string(atom);

        free(reply);

        atom_name_cache[atom] = name;

        return name;
    }
    else
    {
        return iter->second;
    }
}

auto mf::XCBConnection::reply_contains_string_data(xcb_get_property_reply_t const* reply) const -> bool
{
    return reply->type == XCB_ATOM_STRING || reply->type == utf8_string || reply->type == compound_text;
}

auto mf::XCBConnection::string_from(xcb_get_property_reply_t const* reply) const -> std::string
{
    if (!reply_contains_string_data(reply))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Supplied reply is of type \"" + query_name(reply->type) + "\" and does not hold string data"));
    }

    auto const result = std::string{
        static_cast<const char *>(xcb_get_property_value(reply)),
        static_cast<unsigned long>(xcb_get_property_value_length(reply))};
    auto const end = result.find('\0');
    if (end == std::string::npos)
        return result;
    else
        return result.substr(0, end);
}

bool mf::XCBConnection::is_ours(uint32_t id) const
{
    auto setup = xcb_get_setup(xcb_connection);
    return (id & ~setup->resource_id_mask) == setup->resource_id_base;
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(xcb_get_property_reply_t*)> action,
    std::function<void()> on_error) const -> std::function<void()>
{
    xcb_get_property_cookie_t cookie = xcb_get_property(
        xcb_connection,
        0, // don't delete
        window,
        prop,
        XCB_ATOM_ANY,
        0, // no offset
        2048); // big buffer

    return [xcb_connection = xcb_connection, cookie, action, on_error, prop, this]()
        {
            xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection, cookie, nullptr);
            if (reply && reply->type != XCB_ATOM_NONE)
            {
                try
                {
                    action(reply);
                }
                catch (...)
                {
                    log(
                        logging::Severity::warning,
                        MIR_LOG_COMPONENT,
                        std::current_exception(),
                        "Failed to process property reply: " + query_name(prop));
                }
            }
            else
            {
                try
                {
                    on_error();
                }
                catch (...)
                {
                    log(
                        logging::Severity::warning,
                        MIR_LOG_COMPONENT,
                        std::current_exception(),
                        "Failed to run property error callback.");
                }
            }
            free(reply);
        };
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(std::string const&)> action,
    std::function<void()> on_error) const -> std::function<void()>
{
    return read_property(
        window,
        prop,
        [this, action](xcb_get_property_reply_t const* reply)
        {
            action(string_from(reply));
        },
        on_error);
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(uint32_t)> action,
    std::function<void()> on_error) const -> std::function<void()>
{
    return read_property(
        window,
        prop,
        [this, prop, action](xcb_get_property_reply_t const* reply)
        {
            uint8_t const expected_format = 32;
            if (reply->format != expected_format)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Failed to read " + query_name(prop) + " window property." +
                    " Reply of type " + query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected " + std::to_string(expected_format)));
            }

            // The length returned by xcb_get_property_value_length() is in bytes for some reason
            size_t const len = xcb_get_property_value_length(reply) / sizeof(uint32_t);

            if (len != 1)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Failed to read " + query_name(prop) + " window property." +
                    " Reply of type " + query_name(reply->type) +
                    " has a value length " + std::to_string(len) +
                    " instead of expected 1"));
            }

            action(*static_cast<uint32_t const*>(xcb_get_property_value(reply)));
        },
        on_error);
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(std::vector<uint32_t>)> action,
    std::function<void()> on_error) const -> std::function<void()>
{
    return read_property(
        window,
        prop,
        [this, action](xcb_get_property_reply_t const* reply)
        {
            uint8_t const expected_format = 32;
            if (reply->format != expected_format)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Reply of type " + query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected " + std::to_string(expected_format)));
            }

            auto const start = static_cast<uint32_t const*>(xcb_get_property_value(reply));
            // The length returned by xcb_get_property_value_length() is in bytes for some reason
            size_t const len = xcb_get_property_value_length(reply) / sizeof(uint32_t);
            auto const end = start + len;
            std::vector<uint32_t> const values{start, end};

            action(values);
        },
        on_error);
}

void mf::XCBConnection::configure_window(
    xcb_window_t window,
    std::experimental::optional<geometry::Point> position,
    std::experimental::optional<geometry::Size> size,
    std::experimental::optional<xcb_window_t> sibling,
    std::experimental::optional<xcb_stack_mode_t> stack_mode) const
{
    std::vector<uint32_t> values;
    uint32_t mask = 0;

    if (position)
    {
        mask |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
        values.push_back(position.value().x.as_int());
        values.push_back(position.value().y.as_int());
    }

    if (size)
    {
        mask |= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        values.push_back(size.value().width.as_int());
        values.push_back(size.value().height.as_int());
    }

    if (sibling)
    {
        mask |= XCB_CONFIG_WINDOW_SIBLING;
        values.push_back(sibling.value());
    }

    if (stack_mode)
    {
        mask |= XCB_CONFIG_WINDOW_STACK_MODE;
        values.push_back(stack_mode.value());
    }

    if (!values.empty())
    {
        xcb_configure_window(xcb_connection, window, mask, values.data());
    }
}

auto mf::XCBConnection::reply_debug_string(xcb_get_property_reply_t* reply) const -> std::string
{
    if (reply == nullptr)
    {
        return "(null reply)";
    }
    else if (reply_contains_string_data(reply))
    {
        auto const text = string_from(reply);
        return "\"" + text + "\"";
    }
    else
    {
        // The length returned by xcb_get_property_value_length() is in bytes for some reason
        size_t const len = xcb_get_property_value_length(reply) / (reply->format / 8);
        std::stringstream ss;
        ss << std::to_string(reply->format) << "bit " << query_name(reply->type) << "[" << len << "]";
        if (len < 32 && (
            reply->type == XCB_ATOM_CARDINAL ||
            reply->type == XCB_ATOM_INTEGER ||
            reply->type == XCB_ATOM_ATOM ||
            reply->type == XCB_ATOM_WINDOW))
        {
            ss << ": ";
            void* const ptr = xcb_get_property_value(reply);
            switch (reply->type)
            {
            case XCB_ATOM_CARDINAL: // unsigned number
                switch (reply->format)
                {
                case 8: ss << data_buffer_to_debug_string(static_cast<uint8_t*>(ptr), len); break;
                case 16: ss << data_buffer_to_debug_string(static_cast<uint16_t*>(ptr), len); break;
                case 32: ss << data_buffer_to_debug_string(static_cast<uint32_t*>(ptr), len); break;
                }
                break;

            case XCB_ATOM_INTEGER: // signed number
                switch (reply->format)
                {
                case 8: ss << data_buffer_to_debug_string(static_cast<int8_t*>(ptr), len); break;
                case 16: ss << data_buffer_to_debug_string(static_cast<int16_t*>(ptr), len); break;
                case 32: ss << data_buffer_to_debug_string(static_cast<int32_t*>(ptr), len); break;
                }
                break;

            case XCB_ATOM_ATOM:
                if (reply->format != sizeof(xcb_atom_t) * 8)
                {
                    ss << "Atom property has format " << std::to_string(reply->format);
                    break;
                }
                ss << data_buffer_to_debug_string<xcb_atom_t, std::string>(
                    static_cast<xcb_atom_t*>(ptr),
                    len,
                    [this](xcb_atom_t atom) -> std::string
                    {
                        return query_name(atom);
                    });
                break;

            case XCB_ATOM_WINDOW:
                if (reply->format != sizeof(xcb_window_t) * 8)
                {
                    ss << "Window property has format " << std::to_string(reply->format);
                    break;
                }
                ss << data_buffer_to_debug_string<xcb_window_t, std::string>(
                    static_cast<xcb_window_t*>(ptr),
                    len,
                    [this](xcb_window_t window) -> std::string
                    {
                        return window_debug_string(window);
                    });
                break;
            }
        }
        return ss.str();
    }
}

auto mf::XCBConnection::client_message_debug_string(xcb_client_message_event_t* event) const -> std::string
{
    switch (event->format)
    {
    case 8: return data_buffer_to_debug_string(event->data.data8, 20);
    case 16: return data_buffer_to_debug_string(event->data.data16, 10);
    case 32: return data_buffer_to_debug_string(event->data.data32, 5);
    }
    return "unknown format " + std::to_string(event->format);
}

auto mf::XCBConnection::window_debug_string(xcb_window_t window) const -> std::string
{
    if (window == XCB_WINDOW_NONE)
        return "null window";
    else if (window == xcb_screen->root)
        return "root window";
    else if (is_ours(window))
        return "our window " + number_to_readable_name(window);
    else
        return "window " + number_to_readable_name(window);
}

auto mf::XCBConnection::error_debug_string(xcb_generic_error_t* error) const -> std::string
{
    std::string result;

    switch (error->error_code)
    {
    case XCB_REQUEST:
        result = "not a valid request";
        break;

    case XCB_VALUE:
    {
        auto const value_error{reinterpret_cast<xcb_value_error_t*>(error)};
        result += std::to_string(value_error->bad_value) + " is an invalid value";
        break;
    }

    case XCB_WINDOW:
    {
        auto const window_error{reinterpret_cast<xcb_window_error_t*>(error)};
        result +=
            window_debug_string(window_error->bad_value) +
            " (ID " + std::to_string(window_error->bad_value) + ") does not exist";
        break;
    }
    case XCB_ATOM:
    {
        auto const atom_error{reinterpret_cast<xcb_atom_error_t*>(error)};
        result += "atom " + std::to_string(atom_error->bad_value) + " does not exist";
        break;
    }
    default:
        break;
    }

    result +=
        " (error code " + std::to_string(error->error_code) +
        ", opcode " + std::to_string(error->major_code) + "." + std::to_string(error->minor_code) + ")";

    return result;
}

auto mf::XCBConnection::xcb_type_atom(XCBType type) const -> xcb_atom_t
{
    switch (type)
    {
        case XCBType::ATOM:         return XCB_ATOM_ATOM;
        case XCBType::WINDOW:       return XCB_ATOM_WINDOW;
        case XCBType::CARDINAL32:   return XCB_ATOM_CARDINAL;
        case XCBType::STRING:       return XCB_ATOM_STRING;
        case XCBType::UTF8_STRING:  return utf8_string;
        case XCBType::WM_STATE:     return wm_state;
        case XCBType::WM_PROTOCOLS: return wm_protocols;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Invalid XCB type " +
        std::to_string(static_cast<std::underlying_type<XCBType>::type>(type))));
}
