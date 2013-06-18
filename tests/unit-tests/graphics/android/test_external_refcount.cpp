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

#include <functional>
#include <atomic>
#include <memory>
namespace mir
{
namespace graphics
{
namespace android
{

struct MirNativeBuffer;
static void incref_hook(struct android_native_base_t* base);
static void decref_hook(struct android_native_base_t* base);


struct MirNativeBuffer : public ANativeWindowBuffer
{
    MirNativeBuffer(std::function<void(MirNativeBuffer*)> free)
        : free_fn(free),
          mir_reference(true),
          external_references(0)
    {
        common.incRef = incref_hook;
        common.decRef = decref_hook;
    }
    void external_reference()
    {
        external_references++;
    }

    void external_dereference()
    {
        external_references--;
        if ((external_references == 0) && (!mir_reference))
        {
            free_fn(this);
            delete this;
        }
    }

    void internal_dereference()
    {
        mir_reference = false;
        if (external_references == 0)
        {
            free_fn(this);
            delete this;
        }
    }

    private:
    std::function<void(MirNativeBuffer*)> free_fn;
    std::atomic<bool> mir_reference;
    std::atomic<int> external_references;

    ~MirNativeBuffer() = default;
};

static void incref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<MirNativeBuffer*>(base);
    buffer->external_reference();
}
static void decref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<MirNativeBuffer*>(base);
    buffer->external_dereference();
}

struct ExternalRefDeleter
{ 
    void operator()(MirNativeBuffer* a)
    {
        a->internal_dereference();
    }
};

}
}
}


namespace mga=mir::graphics::android;


TEST(AndroidRefcount, ref_allocation_after_shared_ptr_del)
{
    std::function<void(mga::MirNativeBuffer*)> free;

    mga::MirNativeBuffer* c_ownership = nullptr;
    {
        auto tmp = new mga::MirNativeBuffer(free);
        mga::ExternalRefDeleter del;
        std::shared_ptr<mga::MirNativeBuffer> buffer(tmp, del);
        c_ownership = buffer.get();
        c_ownership->external_reference();
        //Mir system loses its reference
    }
    c_ownership->external_dereference();
    //should delete MirNativeBuffer here
}

TEST(AndroidRefcount, driver_hooks)
{
    int call_count = 0;
    {
        mga::MirNativeBuffer* c_ownership = nullptr;
        {
            auto tmp = new mga::MirNativeBuffer([&](mga::MirNativeBuffer*){call_count++;});
            mga::ExternalRefDeleter del;
            std::shared_ptr<mga::MirNativeBuffer> buffer(tmp, del);
            c_ownership = buffer.get();
            c_ownership->common.incRef(&c_ownership->common);
            //Mir system loses its reference
        }
        c_ownership->common.decRef(&c_ownership->common);
        //should delete MirNativeBuffer here
        EXPECT_EQ(0, call_count);
    }
    EXPECT_EQ(1, call_count);
}
