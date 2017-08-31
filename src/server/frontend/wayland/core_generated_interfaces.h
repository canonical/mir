#include <experimental/optional>
#include <boost/throw_exception.hpp>

#include <wayland-server.h>
#include <wayland-server-protocol.h>

#include "mir/fd.h"

namespace mir
{
namespace frontend
{
namespace wayland
{
class Callback
{
protected:
    Callback(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_callback_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
    }
    virtual ~Callback() = default;


    struct wl_client* const client;
    struct wl_resource* const resource;

};



class Compositor
{
protected:
    Compositor(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_compositor_interface, max_version,
                              this, &Compositor::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_compositor interface"}));
        }
    }
    virtual ~Compositor() = default;

    virtual void create_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;
    virtual void create_region(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;

private:
    static void create_surface_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<Compositor*>(wl_resource_get_user_data(resource));
        me->create_surface(client, resource, id);
    }

    static void create_region_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<Compositor*>(wl_resource_get_user_data(resource));
        me->create_region(client, resource, id);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Compositor*>(data);
        auto resource = wl_resource_create(client, &wl_compositor_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_compositor_interface const vtable;
};

struct wl_compositor_interface const Compositor::vtable = {
    create_surface_thunk,
    create_region_thunk,
};


class ShmPool
{
protected:
    ShmPool(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_shm_pool_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~ShmPool() = default;

    virtual void create_buffer(uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) = 0;
    virtual void destroy() = 0;
    virtual void resize(int32_t size) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void create_buffer_thunk(struct wl_client*, struct wl_resource* resource, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format)
    {
        auto me = reinterpret_cast<ShmPool*>(wl_resource_get_user_data(resource));
        me->create_buffer(id, offset, width, height, stride, format);
    }

    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<ShmPool*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void resize_thunk(struct wl_client*, struct wl_resource* resource, int32_t size)
    {
        auto me = reinterpret_cast<ShmPool*>(wl_resource_get_user_data(resource));
        me->resize(size);
    }

    static struct wl_shm_pool_interface const vtable;
};

struct wl_shm_pool_interface const ShmPool::vtable = {
    create_buffer_thunk,
    destroy_thunk,
    resize_thunk,
};


class Shm
{
protected:
    Shm(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_shm_interface, max_version,
                              this, &Shm::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_shm interface"}));
        }
    }
    virtual ~Shm() = default;

    virtual void create_pool(struct wl_client* client, struct wl_resource* resource, uint32_t id, mir::Fd fd, int32_t size) = 0;

private:
    static void create_pool_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, int fd, int32_t size)
    {
        auto me = reinterpret_cast<Shm*>(wl_resource_get_user_data(resource));
        mir::Fd fd_resolved{fd};
        me->create_pool(client, resource, id, fd_resolved, size);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Shm*>(data);
        auto resource = wl_resource_create(client, &wl_shm_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_shm_interface const vtable;
};

struct wl_shm_interface const Shm::vtable = {
    create_pool_thunk,
};


class Buffer
{
protected:
    Buffer(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_buffer_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Buffer() = default;

    virtual void destroy() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Buffer*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static struct wl_buffer_interface const vtable;
};

struct wl_buffer_interface const Buffer::vtable = {
    destroy_thunk,
};


class DataOffer
{
protected:
    DataOffer(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_data_offer_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~DataOffer() = default;

    virtual void accept(uint32_t serial, std::experimental::optional<std::string> const& mime_type) = 0;
    virtual void receive(std::string const& mime_type, mir::Fd fd) = 0;
    virtual void destroy() = 0;
    virtual void finish() = 0;
    virtual void set_actions(uint32_t dnd_actions, uint32_t preferred_action) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void accept_thunk(struct wl_client*, struct wl_resource* resource, uint32_t serial, char const* mime_type)
    {
        auto me = reinterpret_cast<DataOffer*>(wl_resource_get_user_data(resource));
        std::experimental::optional<std::string> mime_type_resolved;
        if (mime_type != nullptr)
        {
            mime_type_resolved = std::experimental::make_optional<std::string>(mime_type);
        }
        me->accept(serial, mime_type_resolved);
    }

    static void receive_thunk(struct wl_client*, struct wl_resource* resource, char const* mime_type, int fd)
    {
        auto me = reinterpret_cast<DataOffer*>(wl_resource_get_user_data(resource));
        mir::Fd fd_resolved{fd};
        me->receive(mime_type, fd_resolved);
    }

    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<DataOffer*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void finish_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<DataOffer*>(wl_resource_get_user_data(resource));
        me->finish();
    }

    static void set_actions_thunk(struct wl_client*, struct wl_resource* resource, uint32_t dnd_actions, uint32_t preferred_action)
    {
        auto me = reinterpret_cast<DataOffer*>(wl_resource_get_user_data(resource));
        me->set_actions(dnd_actions, preferred_action);
    }

    static struct wl_data_offer_interface const vtable;
};

struct wl_data_offer_interface const DataOffer::vtable = {
    accept_thunk,
    receive_thunk,
    destroy_thunk,
    finish_thunk,
    set_actions_thunk,
};


class DataSource
{
protected:
    DataSource(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_data_source_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~DataSource() = default;

    virtual void offer(std::string const& mime_type) = 0;
    virtual void destroy() = 0;
    virtual void set_actions(uint32_t dnd_actions) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void offer_thunk(struct wl_client*, struct wl_resource* resource, char const* mime_type)
    {
        auto me = reinterpret_cast<DataSource*>(wl_resource_get_user_data(resource));
        me->offer(mime_type);
    }

    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<DataSource*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void set_actions_thunk(struct wl_client*, struct wl_resource* resource, uint32_t dnd_actions)
    {
        auto me = reinterpret_cast<DataSource*>(wl_resource_get_user_data(resource));
        me->set_actions(dnd_actions);
    }

    static struct wl_data_source_interface const vtable;
};

struct wl_data_source_interface const DataSource::vtable = {
    offer_thunk,
    destroy_thunk,
    set_actions_thunk,
};


class DataDevice
{
protected:
    DataDevice(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_data_device_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~DataDevice() = default;

    virtual void start_drag(std::experimental::optional<struct wl_resource*> const& source, struct wl_resource* origin, std::experimental::optional<struct wl_resource*> const& icon, uint32_t serial) = 0;
    virtual void set_selection(std::experimental::optional<struct wl_resource*> const& source, uint32_t serial) = 0;
    virtual void release() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void start_drag_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* source, struct wl_resource* origin, struct wl_resource* icon, uint32_t serial)
    {
        auto me = reinterpret_cast<DataDevice*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> source_resolved;
        if (source != nullptr)
        {
            source_resolved = source;
        }
        std::experimental::optional<struct wl_resource*> icon_resolved;
        if (icon != nullptr)
        {
            icon_resolved = icon;
        }
        me->start_drag(source_resolved, origin, icon_resolved, serial);
    }

    static void set_selection_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* source, uint32_t serial)
    {
        auto me = reinterpret_cast<DataDevice*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> source_resolved;
        if (source != nullptr)
        {
            source_resolved = source;
        }
        me->set_selection(source_resolved, serial);
    }

    static void release_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<DataDevice*>(wl_resource_get_user_data(resource));
        me->release();
    }

    static struct wl_data_device_interface const vtable;
};

struct wl_data_device_interface const DataDevice::vtable = {
    start_drag_thunk,
    set_selection_thunk,
    release_thunk,
};


class DataDeviceManager
{
protected:
    DataDeviceManager(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_data_device_manager_interface, max_version,
                              this, &DataDeviceManager::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_data_device_manager interface"}));
        }
    }
    virtual ~DataDeviceManager() = default;

    virtual void create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;
    virtual void get_data_device(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat) = 0;

private:
    static void create_data_source_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<DataDeviceManager*>(wl_resource_get_user_data(resource));
        me->create_data_source(client, resource, id);
    }

    static void get_data_device_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat)
    {
        auto me = reinterpret_cast<DataDeviceManager*>(wl_resource_get_user_data(resource));
        me->get_data_device(client, resource, id, seat);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<DataDeviceManager*>(data);
        auto resource = wl_resource_create(client, &wl_data_device_manager_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_data_device_manager_interface const vtable;
};

struct wl_data_device_manager_interface const DataDeviceManager::vtable = {
    create_data_source_thunk,
    get_data_device_thunk,
};


class Shell
{
protected:
    Shell(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_shell_interface, max_version,
                              this, &Shell::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_shell interface"}));
        }
    }
    virtual ~Shell() = default;

    virtual void get_shell_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface) = 0;

private:
    static void get_shell_surface_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface)
    {
        auto me = reinterpret_cast<Shell*>(wl_resource_get_user_data(resource));
        me->get_shell_surface(client, resource, id, surface);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Shell*>(data);
        auto resource = wl_resource_create(client, &wl_shell_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_shell_interface const vtable;
};

struct wl_shell_interface const Shell::vtable = {
    get_shell_surface_thunk,
};


class ShellSurface
{
protected:
    ShellSurface(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_shell_surface_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~ShellSurface() = default;

    virtual void pong(uint32_t serial) = 0;
    virtual void move(struct wl_resource* seat, uint32_t serial) = 0;
    virtual void resize(struct wl_resource* seat, uint32_t serial, uint32_t edges) = 0;
    virtual void set_toplevel() = 0;
    virtual void set_transient(struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags) = 0;
    virtual void set_fullscreen(uint32_t method, uint32_t framerate, std::experimental::optional<struct wl_resource*> const& output) = 0;
    virtual void set_popup(struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags) = 0;
    virtual void set_maximized(std::experimental::optional<struct wl_resource*> const& output) = 0;
    virtual void set_title(std::string const& title) = 0;
    virtual void set_class(std::string const& class_) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void pong_thunk(struct wl_client*, struct wl_resource* resource, uint32_t serial)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->pong(serial);
    }

    static void move_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->move(seat, serial);
    }

    static void resize_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->resize(seat, serial, edges);
    }

    static void set_toplevel_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->set_toplevel();
    }

    static void set_transient_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->set_transient(parent, x, y, flags);
    }

    static void set_fullscreen_thunk(struct wl_client*, struct wl_resource* resource, uint32_t method, uint32_t framerate, struct wl_resource* output)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> output_resolved;
        if (output != nullptr)
        {
            output_resolved = output;
        }
        me->set_fullscreen(method, framerate, output_resolved);
    }

    static void set_popup_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->set_popup(seat, serial, parent, x, y, flags);
    }

    static void set_maximized_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* output)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> output_resolved;
        if (output != nullptr)
        {
            output_resolved = output;
        }
        me->set_maximized(output_resolved);
    }

    static void set_title_thunk(struct wl_client*, struct wl_resource* resource, char const* title)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->set_title(title);
    }

    static void set_class_thunk(struct wl_client*, struct wl_resource* resource, char const* class_)
    {
        auto me = reinterpret_cast<ShellSurface*>(wl_resource_get_user_data(resource));
        me->set_class(class_);
    }

    static struct wl_shell_surface_interface const vtable;
};

struct wl_shell_surface_interface const ShellSurface::vtable = {
    pong_thunk,
    move_thunk,
    resize_thunk,
    set_toplevel_thunk,
    set_transient_thunk,
    set_fullscreen_thunk,
    set_popup_thunk,
    set_maximized_thunk,
    set_title_thunk,
    set_class_thunk,
};


class Surface
{
protected:
    Surface(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Surface() = default;

    virtual void destroy() = 0;
    virtual void attach(std::experimental::optional<struct wl_resource*> const& buffer, int32_t x, int32_t y) = 0;
    virtual void damage(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void frame(uint32_t callback) = 0;
    virtual void set_opaque_region(std::experimental::optional<struct wl_resource*> const& region) = 0;
    virtual void set_input_region(std::experimental::optional<struct wl_resource*> const& region) = 0;
    virtual void commit() = 0;
    virtual void set_buffer_transform(int32_t transform) = 0;
    virtual void set_buffer_scale(int32_t scale) = 0;
    virtual void damage_buffer(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void attach_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* buffer, int32_t x, int32_t y)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> buffer_resolved;
        if (buffer != nullptr)
        {
            buffer_resolved = buffer;
        }
        me->attach(buffer_resolved, x, y);
    }

    static void damage_thunk(struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->damage(x, y, width, height);
    }

    static void frame_thunk(struct wl_client*, struct wl_resource* resource, uint32_t callback)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->frame(callback);
    }

    static void set_opaque_region_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* region)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> region_resolved;
        if (region != nullptr)
        {
            region_resolved = region;
        }
        me->set_opaque_region(region_resolved);
    }

    static void set_input_region_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* region)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> region_resolved;
        if (region != nullptr)
        {
            region_resolved = region;
        }
        me->set_input_region(region_resolved);
    }

    static void commit_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->commit();
    }

    static void set_buffer_transform_thunk(struct wl_client*, struct wl_resource* resource, int32_t transform)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->set_buffer_transform(transform);
    }

    static void set_buffer_scale_thunk(struct wl_client*, struct wl_resource* resource, int32_t scale)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->set_buffer_scale(scale);
    }

    static void damage_buffer_thunk(struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        auto me = reinterpret_cast<Surface*>(wl_resource_get_user_data(resource));
        me->damage_buffer(x, y, width, height);
    }

    static struct wl_surface_interface const vtable;
};

struct wl_surface_interface const Surface::vtable = {
    destroy_thunk,
    attach_thunk,
    damage_thunk,
    frame_thunk,
    set_opaque_region_thunk,
    set_input_region_thunk,
    commit_thunk,
    set_buffer_transform_thunk,
    set_buffer_scale_thunk,
    damage_buffer_thunk,
};


class Seat
{
protected:
    Seat(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_seat_interface, max_version,
                              this, &Seat::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_seat interface"}));
        }
    }
    virtual ~Seat() = default;

    virtual void get_pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;
    virtual void get_keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;
    virtual void get_touch(struct wl_client* client, struct wl_resource* resource, uint32_t id) = 0;
    virtual void release(struct wl_client* client, struct wl_resource* resource) = 0;

private:
    static void get_pointer_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<Seat*>(wl_resource_get_user_data(resource));
        me->get_pointer(client, resource, id);
    }

    static void get_keyboard_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<Seat*>(wl_resource_get_user_data(resource));
        me->get_keyboard(client, resource, id);
    }

    static void get_touch_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        auto me = reinterpret_cast<Seat*>(wl_resource_get_user_data(resource));
        me->get_touch(client, resource, id);
    }

    static void release_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Seat*>(wl_resource_get_user_data(resource));
        me->release(client, resource);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Seat*>(data);
        auto resource = wl_resource_create(client, &wl_seat_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_seat_interface const vtable;
};

struct wl_seat_interface const Seat::vtable = {
    get_pointer_thunk,
    get_keyboard_thunk,
    get_touch_thunk,
    release_thunk,
};


class Pointer
{
protected:
    Pointer(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_pointer_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Pointer() = default;

    virtual void set_cursor(uint32_t serial, std::experimental::optional<struct wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y) = 0;
    virtual void release() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void set_cursor_thunk(struct wl_client*, struct wl_resource* resource, uint32_t serial, struct wl_resource* surface, int32_t hotspot_x, int32_t hotspot_y)
    {
        auto me = reinterpret_cast<Pointer*>(wl_resource_get_user_data(resource));
        std::experimental::optional<struct wl_resource*> surface_resolved;
        if (surface != nullptr)
        {
            surface_resolved = surface;
        }
        me->set_cursor(serial, surface_resolved, hotspot_x, hotspot_y);
    }

    static void release_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Pointer*>(wl_resource_get_user_data(resource));
        me->release();
    }

    static struct wl_pointer_interface const vtable;
};

struct wl_pointer_interface const Pointer::vtable = {
    set_cursor_thunk,
    release_thunk,
};


class Keyboard
{
protected:
    Keyboard(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_keyboard_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Keyboard() = default;

    virtual void release() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void release_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Keyboard*>(wl_resource_get_user_data(resource));
        me->release();
    }

    static struct wl_keyboard_interface const vtable;
};

struct wl_keyboard_interface const Keyboard::vtable = {
    release_thunk,
};


class Touch
{
protected:
    Touch(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Touch() = default;

    virtual void release() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void release_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Touch*>(wl_resource_get_user_data(resource));
        me->release();
    }

    static struct wl_touch_interface const vtable;
};

struct wl_touch_interface const Touch::vtable = {
    release_thunk,
};


class Output
{
protected:
    Output(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_output_interface, max_version,
                              this, &Output::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_output interface"}));
        }
    }
    virtual ~Output() = default;

    virtual void release(struct wl_client* client, struct wl_resource* resource) = 0;

private:
    static void release_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Output*>(wl_resource_get_user_data(resource));
        me->release(client, resource);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Output*>(data);
        auto resource = wl_resource_create(client, &wl_output_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_output_interface const vtable;
};

struct wl_output_interface const Output::vtable = {
    release_thunk,
};


class Region
{
protected:
    Region(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_region_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Region() = default;

    virtual void destroy() = 0;
    virtual void add(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void subtract(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Region*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void add_thunk(struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        auto me = reinterpret_cast<Region*>(wl_resource_get_user_data(resource));
        me->add(x, y, width, height);
    }

    static void subtract_thunk(struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        auto me = reinterpret_cast<Region*>(wl_resource_get_user_data(resource));
        me->subtract(x, y, width, height);
    }

    static struct wl_region_interface const vtable;
};

struct wl_region_interface const Region::vtable = {
    destroy_thunk,
    add_thunk,
    subtract_thunk,
};


class Subcompositor
{
protected:
    Subcompositor(struct wl_display* display, uint32_t max_version)
        : max_version{max_version}
    {
        if (!wl_global_create(display, 
                              &wl_subcompositor_interface, max_version,
                              this, &Subcompositor::bind))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to export wl_subcompositor interface"}));
        }
    }
    virtual ~Subcompositor() = default;

    virtual void destroy(struct wl_client* client, struct wl_resource* resource) = 0;
    virtual void get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface, struct wl_resource* parent) = 0;

private:
    static void destroy_thunk(struct wl_client* client, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Subcompositor*>(wl_resource_get_user_data(resource));
        me->destroy(client, resource);
    }

    static void get_subsurface_thunk(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface, struct wl_resource* parent)
    {
        auto me = reinterpret_cast<Subcompositor*>(wl_resource_get_user_data(resource));
        me->get_subsurface(client, resource, id, surface, parent);
    }

    static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)
    {
        auto me = reinterpret_cast<Subcompositor*>(data);
        auto resource = wl_resource_create(client, &wl_subcompositor_interface,
                                           std::min(version, me->max_version), id);
        if (resource == nullptr)
        {
            wl_client_post_no_memory(client);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, me, nullptr);
    }

    uint32_t const max_version;
    static struct wl_subcompositor_interface const vtable;
};

struct wl_subcompositor_interface const Subcompositor::vtable = {
    destroy_thunk,
    get_subsurface_thunk,
};


class Subsurface
{
protected:
    Subsurface(struct wl_client* client, struct wl_resource* parent, uint32_t id)
        : client{client},
          resource{wl_resource_create(client, &wl_subsurface_interface, wl_resource_get_version(parent), id)}
    {
        if (resource == nullptr)
        {
            wl_resource_post_no_memory(parent);
            BOOST_THROW_EXCEPTION((std::bad_alloc{}));
        }
        wl_resource_set_implementation(resource, &vtable, this, nullptr);
    }
    virtual ~Subsurface() = default;

    virtual void destroy() = 0;
    virtual void set_position(int32_t x, int32_t y) = 0;
    virtual void place_above(struct wl_resource* sibling) = 0;
    virtual void place_below(struct wl_resource* sibling) = 0;
    virtual void set_sync() = 0;
    virtual void set_desync() = 0;

    struct wl_client* const client;
    struct wl_resource* const resource;

private:
    static void destroy_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->destroy();
    }

    static void set_position_thunk(struct wl_client*, struct wl_resource* resource, int32_t x, int32_t y)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->set_position(x, y);
    }

    static void place_above_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* sibling)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->place_above(sibling);
    }

    static void place_below_thunk(struct wl_client*, struct wl_resource* resource, struct wl_resource* sibling)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->place_below(sibling);
    }

    static void set_sync_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->set_sync();
    }

    static void set_desync_thunk(struct wl_client*, struct wl_resource* resource)
    {
        auto me = reinterpret_cast<Subsurface*>(wl_resource_get_user_data(resource));
        me->set_desync();
    }

    static struct wl_subsurface_interface const vtable;
};

struct wl_subsurface_interface const Subsurface::vtable = {
    destroy_thunk,
    set_position_thunk,
    place_above_thunk,
    place_below_thunk,
    set_sync_thunk,
    set_desync_thunk,
};


}
}
}
