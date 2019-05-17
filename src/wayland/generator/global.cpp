/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "global.h"

#include <libxml++/libxml++.h>

Global::Global(std::string const& wl_name, std::string const& generated_name, int version, std::string const& nmspace)
    : wl_name{wl_name},
      version{version},
      generated_name{generated_name},
      nmspace{nmspace}
{
}

Emitter Global::declaration() const
{
    return Lines{
        {"class Global : wayland::Global"},
        "{",
        "public:",
        Emitter::layout(Lines{
            {"Global(", constructor_args(), ");"},
            empty_line,
            {"auto interface_name() const -> char const* override;"}
        }, true, true, Emitter::single_indent),
        empty_line,
        "private:",
        Emitter::layout(Lines{
            bind_prototype(),
            {"friend ", generated_name, "::Thunks;"},
        }, true, true, Emitter::single_indent),
        "};"
    };
}

Emitter Global::implementation() const
{
    return EmptyLineList{
        Lines{
            {nmspace, "Global::Global(", constructor_args(), ")"},
            {"    : wayland::Global{"},
            {"          wl_global_create("},
            {"              display,"},
            {"              &", wl_name, "_interface_data,"},
            {"              Thunks::supported_version,"},
            {"              this,"},
            {"              &Thunks::bind_thunk)}"},
            Block{
            }
        },
        Lines{
            {"auto ", nmspace, "Global::interface_name() const -> char const*"},
            Block{
                {"return ", generated_name, "::interface_name;"},
            }
        }
    };
}

Emitter Global::bind_thunk_impl() const
{
    return Lines{
        "static void bind_thunk(struct wl_client* client, void* data, uint32_t version, uint32_t id)",
        Block{
            {"auto me = static_cast<", generated_name, "::Global*>(data);"},
            {"auto resource = wl_resource_create("},
            Emitter::layout(Lines{
                "client,",
                {"&", wl_name, "_interface_data,"},
                {"std::min((int)version, Thunks::supported_version),"},
                "id);",
            }, true, true, Emitter::single_indent),
            "if (resource == nullptr)",
            Block{
                "wl_client_post_no_memory(client);",
                "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
            },
            "try",
            Block{
                "me->bind(resource);"
            },
            "catch(...)",
            Block{{
                {"internal_error_processing_request(client, \"", generated_name, " global bind\");"},
            }}
        }
    };
}

Emitter Global::constructor_args() const
{
    return {"wl_display* display, Version<", std::to_string(version), ">"};
}

Emitter Global::bind_prototype() const
{
    return {"virtual void bind(wl_resource* new_", wl_name, ") = 0;"};
}
