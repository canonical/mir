/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_buffer.h"
#include <thread>
#include <memory>
#include <chrono>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <mutex>
#include <condition_variable>

int main(int argc, char *argv[])
try
{
    (void) argc; (void) argv;
/*
    sigset_t sigset;
    siginfo_t siginfo;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    sigwaitinfo(&sigset, &siginfo);
*/
    printf("cmon!\n");
    auto connection = mir_connect_sync(nullptr, "caps");

    printf("CONNECT\n");
    auto pf = mir_pixel_format_abgr_8888;
    unsigned int width = 420;
    unsigned int height = 320;
    MirRectangle rect { 0, 0, width, height };
    auto spec = mir_create_screencast_spec(connection);
    mir_screencast_spec_set_width(spec, width);
    mir_screencast_spec_set_height(spec, height);
    mir_screencast_spec_set_capture_region(spec, &rect);
    mir_screencast_spec_set_mirror_mode(spec, mir_mirror_mode_none);
    mir_screencast_spec_set_number_of_buffers(spec, 0);

    //TODO: no
    mir_screencast_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    printf("SAASAS\n");
    auto screencast = mir_screencast_create_sync(spec);
    printf("done SAASAS\n");
    mir_screencast_spec_release(spec);

    struct BufferSync
    {
        MirBuffer* buffer = nullptr;
        std::mutex mutex;
        std::condition_variable cv;
    } sync;
    MirBuffer* buffer = nullptr;

    printf("YEPE \n");
    mir_connection_allocate_buffer(
        connection, width, height, pf, mir_buffer_usage_software,
        [] ( MirBuffer* buffer, void* context)
        {
            auto sync = reinterpret_cast<BufferSync*>(context);
            std::unique_lock<decltype(sync->mutex)> lk(sync->mutex);
            sync->buffer = buffer;
            sync->cv.notify_all();
        }, &sync);

    {
        std::unique_lock<decltype(sync.mutex)> lk(sync.mutex);
        sync.cv.wait(lk, [&] { return sync.buffer; } );
        buffer = sync.buffer;
    }

    struct Capture
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
    } cap;

    printf("CAP\n");
    mir_screencast_capture_to_buffer(screencast, buffer,
        [](MirBuffer*, void* context) {
            printf("BBB\n");
            auto c = reinterpret_cast<Capture*>(context);
            std::unique_lock<std::mutex> lk(c->mutex);
            c->done = true;
            c->cv.notify_all();
            printf("BBBa\n");
        }, &cap);

    printf("BAh\n");
    std::unique_lock<std::mutex> lk(cap.mutex);
    cap.cv.wait(lk, [&] { return cap.done; });
    printf("endCAP\n");

    MirBufferLayout layout;
    MirGraphicsRegion region;
    mir_buffer_map(buffer, &region, &layout);
    printf("MAP IS %i %i\n", region.width, region.height);
    for(auto i = 0; i < region.stride; i++)
    {
        printf("%X ", region.vaddr[i]);
    }
    mir_buffer_unmap(buffer);

//    mir_screencast_release_sync(screencast);
    printf("RELESA\n");
//    mir_connection_release(connection);
    printf("END END\n");
    return 0;
}
catch(std::exception& e)
{
    std::cerr << "error : " << e.what() << std::endl;
    return 1;
}
