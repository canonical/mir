/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 *
 */

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_buffer_stream.h>
#include <mir_toolkit/mir_window.h>
#include <mir_toolkit/mir_presentation_chain.h>
#include <mir_toolkit/rs/mir_render_surface.h>
#include <mir_toolkit/mir_buffer.h>
#include <mir_toolkit/version.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <assert.h>

#define PALETTE_SIZE 5

void fill_buffer_diagonal_stripes(
    MirBuffer* buffer, unsigned int fg, unsigned int bg)
{
    MirBufferLayout layout = mir_buffer_layout_unknown;
    MirGraphicsRegion region;
    mir_buffer_map(buffer, &region, &layout);
    if ((!region.vaddr) || (region.pixel_format != mir_pixel_format_abgr_8888) || layout != mir_buffer_layout_linear)
        return;

    unsigned char* vaddr = (unsigned char*) region.vaddr;
    int const num_stripes = 10;
    int const stripes_thickness = region.width / num_stripes;
    for(int i = 0; i < region.height; i++)
    {
        unsigned int* pixel = (unsigned int*) vaddr;
        for(int j = 0; j < region.width ; j++)
        {
            if ((((i + j) / stripes_thickness) % stripes_thickness) % 2)
                pixel[j] = bg;
            else
                pixel[j] = fg;
        }
        vaddr += region.stride; 
    }
    mir_buffer_unmap(buffer);
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

volatile sig_atomic_t rendering = 1;
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


    int const chain_width = width / 2;
    int const chain_height = height / 2;

    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &signal_set, NULL);

    struct sigaction action;
    action.sa_handler = shutdown;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);


    int displacement_x = 0;
    int displacement_y = 0;

    MirPixelFormat format = mir_pixel_format_abgr_8888;

    MirConnection* connection = mir_connect_sync(socket_file, "prerendered_frames");
    if (!mir_connection_is_valid(connection))
    {
        printf("could not connect to server file at: %s\n", socket_file);
        return -1;
    }

    unsigned int const num_chains = 4;
    unsigned int const num_buffers = num_chains + 1;
    unsigned int const fg[PALETTE_SIZE] = {
        0xFF14BEA0,
        0xFF000000,
        0xFF1111FF,
        0xFFAAAAAA,
        0xFFB00076
    };
    unsigned int const bg[PALETTE_SIZE] = {
        0xFFDF2111,
        0xFFFFFFFF,
        0xFF11DDDD,
        0xFF404040,
        0xFFFFFF00
    };
    unsigned int spare_buffer = 0;

    MirPresentationChain* chain[num_chains];
    MirRenderSurface* render_surface[num_chains];
    for(unsigned int i = 0u; i < num_chains; i++)
    {
        render_surface[i] = mir_connection_create_render_surface_sync(connection, chain_width, chain_height);
        if (!mir_render_surface_is_valid(render_surface[i]))
        {
            printf("could not create render surface\n");
            return -1;
        }

        chain[i] =  mir_render_surface_get_presentation_chain(render_surface[i]);
        if (!mir_presentation_chain_is_valid(chain[i]))
        {
            printf("could not create MirPresentationChain\n");

            return -1;
        }
    }

    //Arrange a 2x2 grid of chains within window
    MirWindowSpec* spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_add_render_surface(
        spec, render_surface[0], chain_width, chain_height, displacement_x, displacement_y);
    mir_window_spec_add_render_surface(
        spec, render_surface[1], chain_width, chain_height, chain_width, displacement_y);
    mir_window_spec_add_render_surface(
        spec, render_surface[2], chain_width, chain_height, displacement_x, chain_height);
    mir_window_spec_add_render_surface(
        spec, render_surface[3], chain_width, chain_height, chain_width, chain_height);
    MirWindow* window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    SubmissionInfo buffer_available[num_buffers];

    //prerender the frames
    for (unsigned int i = 0u; i < num_buffers; i++)
    {
        pthread_cond_init(&buffer_available[i].cv, NULL);
        pthread_mutex_init(&buffer_available[i].lock, NULL);
        buffer_available[i].available = 0;
        buffer_available[i].buffer = NULL;

        mir_connection_allocate_buffer(
            connection, width, height, format, available_callback, &buffer_available[i]);

        pthread_mutex_lock(&buffer_available[i].lock);
        while(!buffer_available[i].buffer)
            pthread_cond_wait(&buffer_available[i].cv, &buffer_available[i].lock);

        fill_buffer_diagonal_stripes(buffer_available[i].buffer,
            fg[i % PALETTE_SIZE], bg[i % PALETTE_SIZE]);

        pthread_mutex_unlock(&buffer_available[i].lock);
    }

    while (rendering)
    {
        for(unsigned int i = 0u; i < num_chains; i++)
        {
            MirBuffer* b;
            pthread_mutex_lock(&buffer_available[spare_buffer].lock);
            while(!buffer_available[spare_buffer].available)
                pthread_cond_wait(&buffer_available[spare_buffer].cv, &buffer_available[spare_buffer].lock);
            buffer_available[spare_buffer].available = 0;
            b = buffer_available[spare_buffer].buffer;
            pthread_mutex_unlock(&buffer_available[spare_buffer].lock);

            mir_presentation_chain_submit_buffer(
                chain[i], b, available_callback, &buffer_available[spare_buffer]);

            //just looks like a blur if we don't slow things down
            ualarm(500000, 0);
            int sig;
            sigwait(&signal_set, &sig);
            if (!rendering) break;

            if (++spare_buffer > num_chains)
                spare_buffer = 0;
        }
    }

    for (unsigned int i = 0u; i < num_buffers; i++)
        mir_buffer_release(buffer_available[i].buffer);
    for (unsigned int i = 0u; i < num_chains; i++)
        mir_render_surface_release(render_surface[i]);
    mir_window_release_sync(window);
    mir_connection_release(connection);
    return 0;
}
