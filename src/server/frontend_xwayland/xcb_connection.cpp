/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
 * Copyright (C) Canonical Ltd.
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

#include "xwayland_log.h"
#include "mir/log.h"
#include "mir/c_memory.h"

#include "boost/throw_exception.hpp"
#include <sstream>
#include <xcb/res.h>

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
    return "unknown XCB error " + std::to_string(error);
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

template<typename T>
auto value_handler(
    mf::XCBConnection const* connection,
    xcb_atom_t prop,
    mf::XCBConnection::Handler<T> handler) -> mf::XCBConnection::Handler<xcb_get_property_reply_t*>
{
    return {
        [connection, prop, on_success=std::move(handler.on_success)](xcb_get_property_reply_t const* reply)
        {
            uint8_t const expected_format = sizeof(T) * 8;
            if (reply->format != expected_format)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Failed to read " + connection->query_name(prop) + " window property." +
                    " Reply of type " + connection->query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected " + std::to_string(expected_format)));
            }

            // The length returned by xcb_get_property_value_length() is in bytes for some reason
            size_t const byte_len = xcb_get_property_value_length(reply);

            if (byte_len != sizeof(T))
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Failed to read " + connection->query_name(prop) + " window property." +
                    " Reply of type " + connection->query_name(reply->type) +
                    " has a byte length " + std::to_string(byte_len) +
                    " instead of expected " + std::to_string(sizeof(T))));
            }

            on_success(*static_cast<T const*>(xcb_get_property_value(reply)));
        },
        std::move(handler.on_error)
    };
}

template<typename T>
auto vector_handler(
    mf::XCBConnection const* connection,
    xcb_atom_t prop,
    mf::XCBConnection::Handler<std::vector<T>> handler) -> mf::XCBConnection::Handler<xcb_get_property_reply_t*>
{
    return {
        [connection, prop, on_success=handler.on_success](xcb_get_property_reply_t const* reply)
        {
            uint8_t const expected_format = sizeof(T) * 8;
            if (reply->format != expected_format)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Failed to read " + connection->query_name(prop) + " window property." +
                    " Reply of type " + connection->query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected " + std::to_string(expected_format)));
            }

            auto const start = static_cast<T const*>(xcb_get_property_value(reply));
            // The length returned by xcb_get_property_value_length() is in bytes for some reason
            size_t const len = xcb_get_property_value_length(reply) / sizeof(T);
            auto const end = start + len;
            std::vector<T> const values{start, end};

            on_success(values);
        },
        std::move(handler.on_error)
    };
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
        std::lock_guard lock{mutex};

        // The initial atomic check is faster, but we need to check again once we've got the lock
        if (atom == XCB_ATOM_NONE)
        {
            auto const reply = make_unique_cptr(xcb_intern_atom_reply(*connection, cookie, nullptr));
            if (!reply)
                BOOST_THROW_EXCEPTION(std::runtime_error("Failed to look up atom " + name_));
            atom = reply->atom;

            std::lock_guard lock{connection->atom_name_cache_mutex};
            connection->atom_name_cache[atom] = name_;
        }
    }

    return atom;
}

mf::XCBConnection::XCBConnection(Fd const& fd)
    : fd{fd},
      xcb_connection{connect_to_fd(fd)},
      xcb_screen{xcb_setup_roots_iterator(xcb_get_setup(xcb_connection)).data},
      atom_name_cache{{XCB_ATOM_NONE, "None/Any"}}
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
    std::lock_guard lock{atom_name_cache_mutex};
    auto const iter = atom_name_cache.find(atom);

    if (iter == atom_name_cache.end())
    {
        xcb_get_atom_name_cookie_t const cookie = xcb_get_atom_name(xcb_connection, atom);
        auto const reply = make_unique_cptr(xcb_get_atom_name_reply(xcb_connection, cookie, nullptr));

        std::string name;

        if (reply)
        {
            name = std::string{
                xcb_get_atom_name_name(reply.get()),
                static_cast<size_t>(xcb_get_atom_name_name_length(reply.get()))};
        }
        else
        {
            name = "Atom " + std::to_string(atom);
        }

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
    return reply->type == XCB_ATOM_STRING || reply->type == UTF8_STRING || reply->type == COMPOUND_TEXT;
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
    bool delete_after_read,
    uint32_t max_length,
    Handler<xcb_get_property_reply_t*>&& handler) const -> std::function<void()>
{
    xcb_get_property_cookie_t cookie = xcb_get_property(
        xcb_connection,
        delete_after_read ? 1 : 0,
        window,
        prop,
        XCB_ATOM_ANY,
        0, // no offset
        max_length);

    return [this, cookie, handler=std::move(handler), window, prop]()
        {
            try
            {
                Error error;
                auto const reply = make_unique_cptr(xcb_get_property_reply(xcb_connection, cookie, &error.ptr));
                if (reply && reply->type != XCB_ATOM_NONE)
                {
                    handler.on_success(reply.get());
                }
                else if (reply)
                {
                    std::string message = "no reply data";
                    if (verbose_xwayland_logging_enabled())
                    {
                        message +=  " for " + window_debug_string(window) + "." + query_name(prop);
                    }
                    handler.on_error(message);
                }
                else
                {
                    std::string message = "error reading property: ";
                    if (verbose_xwayland_logging_enabled())
                    {
                        message = "error reading " + window_debug_string(window) + "." + query_name(prop) + ": ";
                    }
                    handler.on_error(message + error_debug_string(error.ptr));
                }
            }
            catch (...)
            {
                log(
                    logging::Severity::warning,
                    MIR_LOG_COMPONENT,
                    "Exception thrown processing reply for property " +
                    window_debug_string(window) + "." + query_name(prop));
            }
        };
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<xcb_get_property_reply_t*>&& handler) const -> std::function<void()>
{
    return read_property(window, prop, false, 2048, std::move(handler));
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<std::string> handler) const -> std::function<void()>
{
    return read_property(
        window,
        prop,
        {
            [this, on_success=std::move(handler.on_success)](xcb_get_property_reply_t const* reply)
            {
                on_success(string_from(reply));
            },
            std::move(handler.on_error)
        });
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<uint32_t> handler) const -> std::function<void()>
{
    return read_property(window, prop, value_handler(this, prop, handler));
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<int32_t> handler) const -> std::function<void()>
{
    return read_property(window, prop, value_handler(this, prop, handler));
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<std::vector<uint32_t>> handler) const -> std::function<void()>
{
    return read_property(window, prop, vector_handler(this, prop, handler));
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    Handler<std::vector<int32_t>> handler) const -> std::function<void()>
{
    return read_property(window, prop, vector_handler(this, prop, handler));
}

void mf::XCBConnection::configure_window(
    xcb_window_t window,
    std::optional<geometry::Point> position,
    std::optional<geometry::Size> size,
    std::optional<xcb_window_t> sibling,
    std::optional<xcb_stack_mode_t> stack_mode) const
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

auto mf::XCBConnection::query_client_pid(
    xcb_window_t window,
    Handler<uint32_t> handler) const -> std::function<void()>
{
    xcb_res_client_id_spec_t spec = {
	.client = window,
	.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID,
    };
    xcb_res_query_client_ids_cookie_t cookie = xcb_res_query_client_ids(
	xcb_connection,
	1,
	&spec);

    return [this, cookie, handler=std::move(handler), window]()
        {
            try
            {
                Error error;
		auto const reply = make_unique_cptr(xcb_res_query_client_ids_reply(xcb_connection, cookie, &error.ptr));
		if (reply)
		{
		    for (auto i = xcb_res_query_client_ids_ids_iterator(reply.get()); i.rem; xcb_res_client_id_value_next(&i))
		    {
			if (i.data->spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID)
			{
			    uint32_t *pid = xcb_res_client_id_value_value(i.data);
			    handler.on_success(*pid);
			    return;
			}
		    }
		    // No returned local client pid
		    std::string message = "client is not local";
		    handler.on_error(message);
		}
                else
                {
                    std::string message = "error querying client pid: ";
                    if (verbose_xwayland_logging_enabled())
                    {
                        message = "error querying client pid for " + window_debug_string(window) + ": ";
                    }
                    handler.on_error(message + error_debug_string(error.ptr));
                }
            }
            catch (...)
            {
                log(
                    logging::Severity::warning,
                    MIR_LOG_COMPONENT,
                    "Exception thrown processing reply for querying pid of " +
                    window_debug_string(window));
            }
        };
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
        return "our window " + std::to_string(window);
    else
        return "window " + std::to_string(window);
}

auto mf::XCBConnection::error_debug_string(xcb_generic_error_t* error) const -> std::string
{
    std::string result;

    if (!error)
    {
        return "(given XCB error is null)";
    }

    switch (error->error_code)
    {
    case XCB_REQUEST:
    {
        result = "not a valid request ";
        break;
    }

    case XCB_VALUE:
    {
        auto const value_error{reinterpret_cast<xcb_value_error_t*>(error)};
        result = std::to_string(value_error->bad_value) + " is an invalid value ";
        break;
    }

    case XCB_WINDOW:
    {
        auto const window_error{reinterpret_cast<xcb_window_error_t*>(error)};
        result =
            window_debug_string(window_error->bad_value) +
            " (ID " + std::to_string(window_error->bad_value) + ") does not exist ";
        break;
    }

    case XCB_ATOM:
    {
        auto const atom_error{reinterpret_cast<xcb_atom_error_t*>(error)};
        result = "atom " + std::to_string(atom_error->bad_value) + " does not exist ";
        break;
    }

    default:
        break;
    }

    result +=
        "(error code " + std::to_string(error->error_code) +
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
        case XCBType::INTEGER32:    return XCB_ATOM_INTEGER;
        case XCBType::STRING:       return XCB_ATOM_STRING;
        case XCBType::UTF8_STRING:  return UTF8_STRING;
        case XCBType::WM_STATE:     return WM_STATE;
        case XCBType::WM_PROTOCOLS: return WM_PROTOCOLS;
        case XCBType::INCR:         return INCR;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Invalid XCB type " +
        std::to_string(static_cast<std::underlying_type<XCBType>::type>(type))));
}
