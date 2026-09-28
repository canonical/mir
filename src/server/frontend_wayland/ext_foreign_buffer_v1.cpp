/*
 * Copyright © Canonical Ltd.
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
 */

#include "ext_foreign_buffer_v1.h"
#include "ext_image_capture_v1.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace
{
class ExtForeignBufferManagerV1Global : public mw::ExtForeignBufferManagerV1::Global
{
public:
    ExtForeignBufferManagerV1Global(wl_display* display);

private:
    void bind(wl_resource* new_resource) override;
};

class ExtForeignBufferManagerV1 : public mw::ExtForeignBufferManagerV1
{
public:
    ExtForeignBufferManagerV1(wl_resource* resource) : mw::ExtForeignBufferManagerV1(resource, Version<1>()) {}
    void get_session(wl_resource* session, wl_resource* source);
};

class ExtForeignBufferSessionV1 : public mw::ExtForeignBufferSessionV1, public mf::ExtImageCopyBackendSession
{
public:
    ExtForeignBufferSessionV1(wl_resource* resource, mf::ExtImageCopyBackendFactory const& factory) :
        mw::ExtForeignBufferSessionV1(resource, Version<1>()),
        backend(factory(this, false))
    {}

    void get_buffer(struct wl_resource* buffer) override {}

    void destroy() override {}

private:
    std::shared_ptr<mf::ExtImageCopyBackend> backend;
};
}

ExtForeignBufferManagerV1Global::ExtForeignBufferManagerV1Global(wl_display* display) : Global(display, Version<1>()) {}

void ExtForeignBufferManagerV1Global::bind(wl_resource* new_resource) { new ExtForeignBufferManagerV1(new_resource); }

void ExtForeignBufferManagerV1::get_session(wl_resource* session, wl_resource* source)
{
    auto const source_instance = mf::ExtImageCaptureSourceV1::from_or_throw(source);
    new ExtForeignBufferSessionV1(session, source_instance->backend_factory);
}

auto mf::create_ext_foreign_buffer_manager_v1(wl_display* display, std::shared_ptr<Executor> const&)
    -> std::shared_ptr<mw::ExtForeignBufferManagerV1::Global>
{
    return std::make_shared<ExtForeignBufferManagerV1Global>(display);
}
