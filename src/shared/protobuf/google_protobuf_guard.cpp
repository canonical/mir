/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/protobuf/google_protobuf_guard.h"

#include <google/protobuf/descriptor.h>
#include <mutex>

namespace mir
{
namespace
{
std::once_flag init_flag;
std::once_flag shutdown_flag;

void init_google_protobuf()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

void shutdown_google_protobuf()
{
    google::protobuf::ShutdownProtobufLibrary();
}

// Too clever? The idea is to ensure protbuf version is verified once (on
// the first google_protobuf_guard() call) and memory is released on exit.
struct google_protobuf_guard_t
{
    google_protobuf_guard_t() { std::call_once(init_flag, init_google_protobuf); }
    ~google_protobuf_guard_t() { std::call_once(shutdown_flag, shutdown_google_protobuf); }
};
}
}

void mir::protobuf::google_protobuf_guard()
{
    static google_protobuf_guard_t guard;
}
