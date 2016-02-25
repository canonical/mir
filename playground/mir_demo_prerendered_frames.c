/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 */

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_buffer_stream.h>
#include <mir_toolkit/mir_surface.h>
#include <mir_toolkit/mir_presentation_chain.h>
#include <mir_toolkit/mir_buffer.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

void fill_buffer(MirBuffer* buffer, int shade, int min, int max)
{
    unsigned char val = (unsigned char) (((float) shade / (max-min)) + min) * 0xFF;

    MirGraphicsRegion region = mir_buffer_get_graphics_region(buffer, mir_read_write);
    if (!region.vaddr)
        return;

    unsigned char* px = (unsigned char*) region.vaddr;
    for(int i = 0; i < region.height; i++)
    {
        px += region.stride; 
        for(int j = 0; j < region.width; j++)
        {
            px[j] = val;
        }
    }
}

typedef struct SubmissionInfo
{
    int available;
    MirBuffer* buffer;
    pthread_mutex_t lock;
    pthread_cond_t cv;
} SubmissionInfo;

static void available_callback(MirPresentationChain* stream, MirBuffer* buffer, void* client_context)
{
    (void) stream;
    SubmissionInfo* info = (SubmissionInfo*) client_context;
    pthread_mutex_lock(&info->lock);
    info->available = 1;
    info->buffer = buffer;
    pthread_cond_broadcast(&info->cv);
    pthread_mutex_unlock(&info->lock);
}

volatile int rendering = 1;
static void shutdown(int signum)
{
    if ((signum == SIGTERM) || (signum == SIGINT))
        rendering = 0;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    int width = 20;
    int height = 25;
    int displacement_x = 0;
    int displacement_y = 0;
    MirPixelFormat format = mir_pixel_format_abgr_8888;

    MirConnection* connection = mir_connect_sync(NULL, "prerendered_frames");
    MirPresentationChain* chain =  mir_connection_create_presentation_chain_sync(connection);
    if (!mir_presentation_chain_is_valid(chain))
        return -1;

    MirSurfaceSpec* spec =
         mir_connection_create_spec_for_normal_surface(connection, width, height, format);
    MirSurface* surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    //reassociate for advanced control
    spec = mir_create_surface_spec(connection);
    mir_surface_spec_add_presentation_chain(
        spec, width, height, displacement_x, displacement_y, chain);
    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    int num_prerendered_frames = 20;
    MirBufferUsage usage = mir_buffer_usage_software;
    SubmissionInfo buffer_available[num_prerendered_frames];

    for (int i = 0u; i < num_prerendered_frames; i++)
    {
        pthread_cond_init(&buffer_available[i].cv, NULL);
        pthread_mutex_init(&buffer_available[i].lock, NULL);
        buffer_available[i].available = 0;
        buffer_available[i].buffer = NULL;

        mir_presentation_chain_allocate_buffer(
            chain, width, height, format, usage, available_callback, &buffer_available[i]);

        pthread_mutex_lock(&buffer_available[i].lock);
        while(!buffer_available[i].buffer)
            pthread_cond_wait(&buffer_available[i].cv, &buffer_available[i].lock);
        fill_buffer(buffer_available[i].buffer, i, 0, num_prerendered_frames);
        pthread_mutex_unlock(&buffer_available[i].lock);
    }

    int i = 0;
    while (rendering)
    {
        MirBuffer* b;
        pthread_mutex_lock(&buffer_available[i].lock);
        while(!buffer_available[i].available)
            pthread_cond_wait(&buffer_available[i].cv, &buffer_available[i].lock);
        buffer_available[i].available = 0;
        b = buffer_available[i].buffer;
        pthread_mutex_unlock(&buffer_available[i].lock);

        mir_presentation_chain_submit_buffer(chain, b);

        i = (i + 1) % num_prerendered_frames;
    }

    for (i = 0u; i < num_prerendered_frames; i++)
        mir_buffer_release(buffer_available[i].buffer);
    mir_presentation_chain_release(chain);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
    return 0;
}
