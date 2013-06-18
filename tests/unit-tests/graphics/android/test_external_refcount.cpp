/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include <gtest/gtest.h>

#include <system/window.h>

#include <memory>
namespace mir
{
namespace graphics
{
namespace android
{
struct MirNativeBuffer : public ANativeWindowBuffer
{
    void external_reference() {}
    void external_dereference() {}
};

struct ExternalRefDeleter
{ 
    void operator()(MirNativeBuffer* )//a)
    {
    }
};



}
}
}


namespace mga=mir::graphics::android;


TEST(AndroidRefcount, ref_allocation_after_shared_ptr_del)
{
    mga::MirNativeBuffer* c_ownership = nullptr;
    {
        auto tmp = new mga::MirNativeBuffer;
        mga::ExternalRefDeleter del;
        std::shared_ptr<mga::MirNativeBuffer> buffer(tmp, del);
        c_ownership = buffer.get();
        c_ownership->external_reference();
        //Mir system loses its reference
    }
    c_ownership->external_dereference();
    //should delete MirNativeBuffer here
}
