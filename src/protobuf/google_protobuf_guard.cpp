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

#include <google/protobuf/descriptor.h>
#include <dlfcn.h>

extern "C" int __attribute__((constructor))
init_google_protobuf()
{
    // Leak libmirprotobuf.so.X
    // This will stop it getting unloaded/reloaded and work around LP: #1391976
    Dl_info self;
    if (dladdr(reinterpret_cast<void*>(&init_google_protobuf), &self))
    {
        dlopen(self.dli_fname, RTLD_LAZY | RTLD_NODELETE);
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    return 0;
}

extern "C" int __attribute__((destructor))
shutdown_google_protobuf()
{
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

// Preserve ABI
namespace mir { namespace protobuf { void google_protobuf_guard(); }}

void mir::protobuf::google_protobuf_guard()
{
}
