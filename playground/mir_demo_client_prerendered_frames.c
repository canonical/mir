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
#include <mir_toolkit/version.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <getopt.h>

float distance(int x0, int y0, int x1, int y1)
{
    float dx = x1 - x0;
    float dy = y1 - y0; 
    return sqrt((dx * dx +  dy * dy));
}

void fill_buffer_with_centered_circle_abgr(
    MirBuffer* buffer, float radius, unsigned int fg, unsigned int bg)
{
    MirGraphicsRegion region = mir_buffer_get_graphics_region(buffer, mir_read_write);
    if ((!region.vaddr) || (region.pixel_format != mir_pixel_format_abgr_8888))
        return;
    int const center_x = region.width / 2;
    int const center_y = region.height / 2; 
    unsigned char* vaddr = (unsigned char*) region.vaddr;
    for(int i = 0; i < region.height; i++)
    {
        unsigned int* pixel = (unsigned int*) vaddr;
        for(int j = 0; j < region.width ; j++)
        {
            int const centered_i = i - center_y;
            int const centered_j = j - center_x; 
            if (distance(0,0, centered_i, centered_j) > radius)
                pixel[j] = bg;
            else
                pixel[j] = fg;
        }
        vaddr += region.stride; 
    }
}

typedef struct SubmissionInfo
{
    int available;
    MirBuffer* buffer;
    pthread_mutex_t lock;
    pthread_cond_t cv;
} SubmissionInfo;

static void available_callback(MirBuffer* buffer, void* client_context)
{
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
    static char const *socket_file = NULL;
    int arg = -1;
    int width = 400;
    int height = 400;
    while ((arg = getopt (argc, argv, "m:s:h:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;
        case 's':
        {
            unsigned int w, h;
            if (sscanf(optarg, "%ux%u", &w, &h) == 2)
            {
                width = w;
                height = h;
            }
            else
            {
                printf("Invalid size: %s, using default size\n", optarg);
            }
            break;
        }
        case 'h':
        case '?':
        default:
            puts(argv[0]);
            printf("Usage:\n");
            printf("    -m <Mir server socket>\n");
            printf("    -s WIDTHxHEIGHT of window\n");
            printf("    -h help dialog\n");
            return -1;
        }
    }

    signal(SIGTERM, shutdown);
    signal(SIGINT, shutdown);

    int displacement_x = 0;
    int displacement_y = 0;
    unsigned int fg = 0xFF1448DD;
    unsigned int bg = 0xFF6F2177;
    MirPixelFormat format = mir_pixel_format_abgr_8888;

    MirConnection* connection = mir_connect_sync(socket_file, "prerendered_frames");
    if (!mir_connection_is_valid(connection))
    {
        printf("could not connect to server file at: %s\n", socket_file);
        return -1;
    }

    MirPresentationChain* chain =  mir_connection_create_presentation_chain_sync(connection);
    if (!mir_presentation_chain_is_valid(chain))
    {
        printf("could not create MirPresentationChain\n");

// TODO this is a frig to pass smoke tests until we support NBS by default
#if (MIR_CLIENT_VERSION <= MIR_VERSION_NUMBER(3, 3, 0))
        printf("This is currently an unreleased API - likely server support is switched off\n");
        return 0;
#else
        return -1;
#endif
    }

    MirSurfaceSpec* spec = mir_connection_create_spec_for_normal_surface(connection, width, height, format);
    mir_surface_spec_add_presentation_chain(
        spec, width, height, displacement_x, displacement_y, chain);
    MirSurface* surface = mir_surface_create_sync(spec);
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

        mir_connection_allocate_buffer(
            connection, width, height, format, usage, available_callback, &buffer_available[i]);

        pthread_mutex_lock(&buffer_available[i].lock);
        while(!buffer_available[i].buffer)
            pthread_cond_wait(&buffer_available[i].cv, &buffer_available[i].lock);

        float max_radius = distance(0, 0, width, height) / 2.0f;
        float radius_i = ((float) i + 1) / num_prerendered_frames * max_radius;
        fill_buffer_with_centered_circle_abgr(buffer_available[i].buffer, radius_i, fg, bg);

        pthread_mutex_unlock(&buffer_available[i].lock);
    }

    int i = 0;
    int inc = -1;
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

        if ((i == num_prerendered_frames - 1) || (i == 0))
            inc *= -1; 
        i = i + inc;
    }

    for (i = 0u; i < num_prerendered_frames; i++)
        mir_buffer_release(buffer_available[i].buffer);
    mir_presentation_chain_release(chain);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
    return 0;
}
