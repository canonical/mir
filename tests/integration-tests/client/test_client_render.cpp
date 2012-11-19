/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/process/process.h"

#include "mir_client/mir_client_library.h"

#include "mir/compositor/buffer_ipc_package.h"
#include "src/frontend/protobuf_socket_communicator.h"
#include "mir/frontend/resource_cache.h"
#include "src/graphics/android/android_buffer.h"
#include "src/graphics/android/android_alloc_adaptor.h"
#include "mir/thread/all.h"

#include "mir_test/stub_server_tool.h"
#include "mir_test/test_server.h"
#include "mir_test/empty_deleter.h"

#include <gmock/gmock.h>

#include <GLES2/gl2.h>
#include <hardware/gralloc.h>

#include <fstream>

#include <dirent.h>
#include <fnmatch.h>

namespace mp=mir::process;
namespace mt=mir::test;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
static int test_width  = 300;
static int test_height = 200;

static const char* proc_dir = "/proc";
static const char* surface_flinger_executable_name = "surfaceflinger";

int surface_flinger_filter(const struct dirent* d)
{
    if (fnmatch("[1-9]*", d->d_name, 0))
        return 0;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s/cmdline", proc_dir, d->d_name);

    std::ifstream in(path);
    std::string line;

    while(std::getline(in, line))
    {
        if (line.find(surface_flinger_executable_name) != std::string::npos)
            return 1;
    }

    return 0;
}

bool is_surface_flinger_running()
{
    struct dirent **namelist;
    return 0 < scandir(proc_dir, &namelist, surface_flinger_filter, 0);
}

/* used by both client/server for patterns */
bool check_solid_pattern(const std::shared_ptr<MirGraphicsRegion> &region, uint32_t value)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        return false;

    int *pixel = (int*) region->vaddr;
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            if (pixel[j*region->width + i] != (int) value)
            {
                return false;
            }
        }
    }
    return true;
}

bool render_solid_pattern(MirGraphicsRegion *region, uint32_t value)
{
    if (region->pixel_format != mir_pixel_format_rgba_8888 )
        return false;

    int *pixel = (int*) region->vaddr;
    int i,j;
    for(i=0; i< region->width; i++)
    {
        for(j=0; j<region->height; j++)
        {
            pixel[j*region->width + i] = value;
        }
    }
    return true;
}

}

namespace mir
{
namespace test
{
/* client code */
static void connected_callback(MirConnection *connection, void* context)
{
    MirConnection** tmp = (MirConnection**) context;
    *tmp = connection;
}
static void create_callback(MirSurface *surface, void*context)
{
    MirSurface** surf = (MirSurface**) context;
    *surf = surface;
}

static void next_callback(MirSurface *, void*)
{
}

struct TestClient
{
    static void sig_handle(int)
    {
    }

    static int render_single()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        /* only use C api */
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            mir_wait_for(mir_connect("./test_socket_surface", "test_renderer",
                                         &connected_callback, &connection));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_rgba_8888;
        mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                          &create_callback, &surface));
        MirGraphicsRegion graphics_region;
        /* grab a buffer*/
        mir_surface_get_graphics_region( surface, &graphics_region);

        /* render pattern */
        render_solid_pattern(&graphics_region, 0x12345678);

        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;
    }

    static int render_double()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        /* only use C api */
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;
        MirGraphicsRegion graphics_region;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            mir_wait_for(mir_connect("./test_socket_surface", "test_renderer",
                                         &connected_callback, &connection));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_rgba_8888;

        mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                          &create_callback, &surface));
        mir_surface_get_graphics_region( surface, &graphics_region);
        render_solid_pattern(&graphics_region, 0x12345678);

        mir_wait_for(mir_surface_next_buffer(surface, &next_callback, (void*) NULL));
        mir_surface_get_graphics_region( surface, &graphics_region);
        render_solid_pattern(&graphics_region, 0x78787878);

        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;
    }

    static int render_accelerated()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        /* only use C api */
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            mir_wait_for(mir_connect("./test_socket_surface", "test_renderer",
                                         &connected_callback, &connection));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_rgba_8888;

        mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                          &create_callback, &surface));

        int major, minor, n;
        EGLDisplay disp;
        EGLContext context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_GREEN_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        EGLNativeDisplayType native_display = (EGLNativeDisplayType)mir_connection_get_egl_native_display(connection);
        EGLNativeWindowType native_window = (EGLNativeWindowType) mir_surface_get_egl_native_window(surface);

        disp = eglGetDisplay(native_display);
        eglInitialize(disp, &major, &minor);

        eglChooseConfig(disp, attribs, &egl_config, 1, &n);
        egl_surface = eglCreateWindowSurface(disp, egl_config, native_window, NULL);
        context = eglCreateContext(disp, egl_config, EGL_NO_CONTEXT, context_attribs);
        eglMakeCurrent(disp, egl_surface, egl_surface, context);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(disp, egl_surface);
        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;

    }

    static int render_accelerated_double()
    {
        if (signal(SIGCONT, sig_handle) == SIG_ERR)
            return -1;
        pause();

        /* only use C api */
        MirConnection* connection = NULL;
        MirSurface* surface;
        MirSurfaceParameters surface_parameters;

         /* establish connection. wait for server to come up */
        while (connection == NULL)
        {
            mir_wait_for(mir_connect("./test_socket_surface", "test_renderer",
                                         &connected_callback, &connection));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        /* make surface */
        surface_parameters.name = "testsurface";
        surface_parameters.width = test_width;
        surface_parameters.height = test_height;
        surface_parameters.pixel_format = mir_pixel_format_rgba_8888;

        mir_wait_for(mir_surface_create( connection, &surface_parameters,
                                          &create_callback, &surface));

        int major, minor, n;
        EGLDisplay disp;
        EGLContext context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_GREEN_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
        EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        EGLNativeDisplayType native_display = (EGLNativeDisplayType)mir_connection_get_egl_native_display(connection);
        EGLNativeWindowType native_window = (EGLNativeWindowType)mir_surface_get_egl_native_window(surface);

        disp = eglGetDisplay(native_display);
        eglInitialize(disp, &major, &minor);

        eglChooseConfig(disp, attribs, &egl_config, 1, &n);
        egl_surface = eglCreateWindowSurface(disp, egl_config, native_window, NULL);
        context = eglCreateContext(disp, egl_config, EGL_NO_CONTEXT, context_attribs);
        eglMakeCurrent(disp, egl_surface, egl_surface, context);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(disp, egl_surface);

        glClearColor(0.0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(disp, egl_surface);

        mir_wait_for(mir_surface_release(surface, &create_callback, &surface));

        /* release */
        mir_connection_release(connection);
        return 0;

    }

    static int exit_function()
    {
        return EXIT_SUCCESS;
    }
};

/* server code */
struct StubServerGenerator : public mt::StubServerTool
{
    StubServerGenerator(const std::shared_ptr<mc::BufferIPCPackage>& pack, int id)
     : package(pack),
        next_received(false),
        next_allowed(false),
        package_id(id)
    {

    }

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(test_width);
        response->set_height(test_height);
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(package_id);

        unsigned int i;
        response->mutable_buffer()->set_fds_on_side_channel(1);
        for(i=0; i<package->ipc_fds.size(); i++)
            response->mutable_buffer()->add_fd(package->ipc_fds[i]);
        for(i=0; i<package->ipc_data.size(); i++)
            response->mutable_buffer()->add_data(package->ipc_data[i]);

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        wait_condition.notify_one();

        done->Run();
    }

    virtual void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done)
    {
        {
            std::unique_lock<std::mutex> lk(next_guard);
            next_received = true;
            next_cv.notify_all();

            while (!next_allowed) {
                allow_cv.wait(lk);
            }
            next_allowed = false;
        }

        response->set_buffer_id(package_id);
        unsigned int i;
        response->set_fds_on_side_channel(1);
        for(i=0; i<package->ipc_fds.size(); i++)
            response->add_fd(package->ipc_fds[i]);
        for(i=0; i<package->ipc_data.size(); i++)
            response->add_data(package->ipc_data[i]);

        done->Run();
    }

    void wait_on_next_buffer()
    {
        std::unique_lock<std::mutex> lk(next_guard);
        while (!next_received)
            next_cv.wait(lk);
        next_received = false;
    }

    void allow_next_continue()
    {
        std::unique_lock<std::mutex> lk(next_guard);
        next_allowed = true;
        allow_cv.notify_all();
        lk.unlock();
    }

    void set_package(const std::shared_ptr<mc::BufferIPCPackage>& pack, int id)
    {
        package = pack;
        package_id = id;
    }

    std::shared_ptr<mc::BufferIPCPackage> package;

    std::mutex next_guard;
    std::condition_variable next_cv;
    std::condition_variable allow_cv;
    bool next_received;
    bool next_allowed;

    int package_id;
};

struct RegionDeleter
{
    RegionDeleter(gralloc_module_t* grmod, native_handle_t* handle)
     :
    grmod(grmod),
    handle(handle)
    {}

    void operator()(MirGraphicsRegion* region)
    {
        grmod->unlock(grmod, handle);
        free(handle);
        delete region;
    }

    gralloc_module_t *grmod;
    native_handle_t *handle;
};

std::shared_ptr<MirGraphicsRegion> get_graphic_region_from_package(std::shared_ptr<mc::BufferIPCPackage> package,
                                                const hw_module_t *hw_module)
{
    native_handle_t* handle;
    handle = (native_handle_t*) malloc(sizeof(int) * ( 3 + package->ipc_data.size() + package->ipc_fds.size() ));
    handle->numInts = package->ipc_data.size();
    handle->numFds  = package->ipc_fds.size();
    int i;
    for(i = 0; i< handle->numFds; i++)
        handle->data[i] = package->ipc_fds[i];
    for(; i < handle->numFds + handle->numInts; i++)
        handle->data[i] = package->ipc_data[i-handle->numFds];

    int *vaddr;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    gralloc_module_t *grmod = (gralloc_module_t*) hw_module;
    grmod->lock(grmod, handle, usage, 0, 0, test_width, test_height, (void**) &vaddr);


    MirGraphicsRegion* region = new MirGraphicsRegion;
    RegionDeleter del(grmod, handle);

    region->vaddr = (char*) vaddr;
    region->width = test_width;
    region->height = test_height;
    region->pixel_format = mir_pixel_format_rgba_8888;

    return std::shared_ptr<MirGraphicsRegion>(region, del);
}

#if 0
bool check_buffer(std::shared_ptr<mc::BufferIPCPackage> package, const hw_module_t *hw_module, uint32_t check_value )
{
    native_handle_t* handle;
    handle = (native_handle_t*) malloc(sizeof(int) * ( 3 + package->ipc_data.size() + package->ipc_fds.size() ));
    handle->numInts = package->ipc_data.size();
    handle->numFds  = package->ipc_fds.size();
    int i;
    for(i = 0; i< handle->numFds; i++)
        handle->data[i] = package->ipc_fds[i];
    for(; i < handle->numFds + handle->numInts; i++)
        handle->data[i] = package->ipc_data[i-handle->numFds];

    int *vaddr;
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    gralloc_module_t *grmod = (gralloc_module_t*) hw_module;
    grmod->lock(grmod, handle, usage, 0, 0, test_width, test_height, (void**) &vaddr);

    MirGraphicsRegion region;
    region.vaddr = (char*) vaddr;
    region.width = test_width;
    region.height = test_height;
    region.pixel_format = mir_pixel_format_rgba_8888;

    auto valid = check_solid_pattern(&region, check_value);
    grmod->unlock(grmod, handle);
    return valid;
}
#endif
}
}

struct TestClientIPCRender : public testing::Test
{
    /* kdub -- some of the (less thoroughly tested) android blob drivers annoyingly keep
       static state about what process they are in. Once you fork, this info is invalid,
       yet the driver uses the info and bad things happen.
       Fork all needed processes before touching the blob! */
    static void SetUpTestCase() {
        render_single_client_process = mp::fork_and_run_in_a_different_process(
            mt::TestClient::render_single,
            mt::TestClient::exit_function);

        render_double_client_process = mp::fork_and_run_in_a_different_process(
            mt::TestClient::render_double,
            mt::TestClient::exit_function);

        second_render_with_same_buffer_client_process
             = mp::fork_and_run_in_a_different_process(
                            mt::TestClient::render_double,
                            mt::TestClient::exit_function);

        render_accelerated_process
             = mp::fork_and_run_in_a_different_process(
                            mt::TestClient::render_accelerated,
                            mt::TestClient::exit_function);

        render_accelerated_process_double
             = mp::fork_and_run_in_a_different_process(
                            mt::TestClient::render_accelerated_double,
                            mt::TestClient::exit_function);
    }

    void SetUp() {
        ASSERT_FALSE(is_surface_flinger_running());

        size = geom::Size{geom::Width{test_width}, geom::Height{test_height}};
        pf = geom::PixelFormat::rgba_8888;

        /* allocate an android buffer */
        int err;
        struct alloc_device_t *alloc_device_raw;
        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
        if (err < 0)
            throw std::runtime_error("Could not open hardware module");
        gralloc_open(hw_module, &alloc_device_raw);
        auto alloc_device = std::shared_ptr<struct alloc_device_t> ( alloc_device_raw, mir::EmptyDeleter());
        auto alloc_adaptor = std::make_shared<mga::AndroidAllocAdaptor>(alloc_device);

        android_buffer = std::make_shared<mga::AndroidBuffer>(alloc_adaptor, size, pf);
        second_android_buffer = std::make_shared<mga::AndroidBuffer>(alloc_adaptor, size, pf);

        package = android_buffer->get_ipc_package();
        second_package = second_android_buffer->get_ipc_package();

        /* start a server */
        mock_server = std::make_shared<mt::StubServerGenerator>(package, 14);
        test_server = std::make_shared<mt::TestServer>("./test_socket_surface", mock_server);
        EXPECT_CALL(*test_server->factory, make_ipc_server()).Times(testing::AtLeast(0));

        test_server->comm.start();
    }

    void TearDown()
    {
        test_server.reset();
    }

    mir::protobuf::Connection response;

    std::shared_ptr<mt::TestServer> test_server;
    std::shared_ptr<mt::StubServerGenerator> mock_server;

    const hw_module_t    *hw_module;
    geom::Size size;
    geom::PixelFormat pf;
    std::shared_ptr<mp::Process> client_process;
    std::shared_ptr<mc::BufferIPCPackage> package;
    std::shared_ptr<mc::BufferIPCPackage> second_package;
    std::shared_ptr<mga::AndroidBuffer> android_buffer;
    std::shared_ptr<mga::AndroidBuffer> second_android_buffer;

    static std::shared_ptr<mp::Process> render_single_client_process;
    static std::shared_ptr<mp::Process> render_double_client_process;
    static std::shared_ptr<mp::Process> second_render_with_same_buffer_client_process;
    static std::shared_ptr<mp::Process> render_accelerated_process;
    static std::shared_ptr<mp::Process> render_accelerated_process_double;
};
std::shared_ptr<mp::Process> TestClientIPCRender::render_single_client_process;
std::shared_ptr<mp::Process> TestClientIPCRender::render_double_client_process;
std::shared_ptr<mp::Process> TestClientIPCRender::second_render_with_same_buffer_client_process;
std::shared_ptr<mp::Process> TestClientIPCRender::render_accelerated_process;
std::shared_ptr<mp::Process> TestClientIPCRender::render_accelerated_process_double;

TEST_F(TestClientIPCRender, test_render_single)
{
    /* activate client */
    render_single_client_process->cont();

    /* wait for client to finish */
    EXPECT_TRUE(render_single_client_process->wait_for_termination().succeeded());

    /* check content */
    auto region = mt::get_graphic_region_from_package(package, hw_module);
    EXPECT_TRUE(check_solid_pattern(region, 0x12345678));
//    EXPECT_TRUE(mt::check_buffer(package, hw_module, 0x12345678));
}

#if 0
TEST_F(TestClientIPCRender, test_render_double)
{
    /* activate client */
    render_double_client_process->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    EXPECT_TRUE(mt::check_buffer(package, hw_module, 0x12345678));

    mock_server->set_package(second_package, 15);

    mock_server->allow_next_continue();
    /* wait for client to finish */
    EXPECT_TRUE(render_double_client_process->wait_for_termination().succeeded());

    /* check content */
    EXPECT_TRUE(mt::check_buffer(second_package, hw_module, 0x78787878));
}

TEST_F(TestClientIPCRender, test_second_render_with_same_buffer)
{
    /* activate client */
    second_render_with_same_buffer_client_process->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(second_render_with_same_buffer_client_process->wait_for_termination().succeeded());

    /* check content */
    EXPECT_TRUE(mt::check_buffer(package, hw_module, 0x78787878));
}

TEST_F(TestClientIPCRender, test_accelerated_render)
{
    /* activate client */
    render_accelerated_process->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process->wait_for_termination().succeeded());

    /* check content */
    EXPECT_TRUE(mt::check_buffer(package, hw_module, 0xFF0000FF));
}

TEST_F(TestClientIPCRender, test_accelerated_render_double)
{
    /* activate client */
    render_accelerated_process_double->cont();

    /* wait for next buffer */
    mock_server->wait_on_next_buffer();
    mock_server->set_package(second_package, 15);
    mock_server->allow_next_continue();

    mock_server->wait_on_next_buffer();
    mock_server->allow_next_continue();

    /* wait for client to finish */
    EXPECT_TRUE(render_accelerated_process_double->wait_for_termination().succeeded());

    /* check content */
    EXPECT_TRUE(mt::check_buffer(package, hw_module, 0xFF0000FF));
    EXPECT_TRUE(mt::check_buffer(second_package, hw_module, 0xFF00FF00));
}
#endif
