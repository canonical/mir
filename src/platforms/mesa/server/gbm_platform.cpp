/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "gbm_platform.h"
#include "mir/graphics/nested_context.h"
#include "buffer_allocator.h"
#include "ipc_operations.h"
#include "nested_authentication.h"

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

mgm::GBMPlatform::GBMPlatform(
    BypassOption bypass_option,
    std::shared_ptr<mg::NestedContext> const& nested_context) :
    bypass_option(bypass_option),
    nested_context(nested_context),
    gbm{std::make_shared<mgm::helpers::GBMHelper>()}
{
    //note, maybe take mesaauthcontetx
    auto auth = nested_context->auth_extension();
    if (auth.is_set())
    {
        gbm->setup(auth.value()->auth_fd());
    }
    else
    {
        //throw
    }
}


mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgm::GBMPlatform::create_buffer_allocator()
{
    return make_module_ptr<mgm::BufferAllocator>(gbm->device, bypass_option, mgm::BufferImportMethod::gbm_native_pixmap);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgm::GBMPlatform::make_ipc_operations() const
{
    return mir::make_module_ptr<mgm::IpcOperations>(
        std::make_shared<mgm::NestedAuthentication>(nested_context));
}
