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

#include <mir_toolkit/mir_buffer_stream_nbs.h>
#include <mir_toolkit/mir_buffer.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

void fill_buffer(MirBuffer* buffer, int shade, int min, int max)
{
    unsigned char val = (unsigned char) (((float) shade / (max-min)) + min) * 0xFF;
    
    MirGraphicsRegion* region = mir_buffer_lock(buffer, mir_write_fence);
    if (!region)
        return;

    unsigned char* px = (unsigned char*) region->vaddr;
    for(int i = 0; i < region->height; i++)
    {
        px += region->stride; 
        for(int j = 0; j < region->width; j++)
        {
            px[j] = val;
        }
    }
    mir_buffer_unlock(region);
}

typedef struct SubmissionInfo
{
    int available;
    MirBuffer* buffer;
    pthread_mutex_t lock;
    pthread_cond_t cv;
} SubmissionInfo;

static void available_callback(MirBufferStream* stream, MirBuffer* buffer, void* client_context)
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
static void sig_handler(int signum)
{
    if ((signum == SIGTERM) || (signum == SIGINT))
        rendering = 0;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sig_handler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    //TODO: add connection stuff
    MirBufferStream* stream = NULL;

    int num_prerendered_frames = 20;
    int width = 20;
    int height = 25;
    MirBufferUsage usage = mir_buffer_usage_software;
    MirPixelFormat format = mir_pixel_format_abgr_8888;

    SubmissionInfo buffer_available[num_prerendered_frames];

    for (int i = 0u; i < num_prerendered_frames; i++)
    {
        pthread_cond_init(&buffer_available[i].cv, NULL);
        pthread_mutex_init(&buffer_available[i].lock, NULL);
        buffer_available[i].available = 0;
        buffer_available[i].buffer = NULL;

        mir_buffer_stream_allocate_buffer(
            stream, width, height, format, usage, available_callback, &buffer_available[i]);

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

        if (!mir_buffer_stream_submit_buffer(stream, b))
            rendering = false;

        i = (i + 1) % num_prerendered_frames;
    }

    for (i = 0u; i < num_prerendered_frames; i++)
        mir_buffer_stream_release_buffer(stream, buffer_available[i].buffer);

    //TODO: connection shutdown stuff
    return 0;
}
