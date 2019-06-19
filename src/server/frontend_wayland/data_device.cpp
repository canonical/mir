/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "data_device.h"
#include "wl_seat.h"

#include <vector>
#include <algorithm>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace mir
{
namespace wayland
{
extern struct wl_interface const wl_data_offer_interface_data;
}
}

namespace
{
class DataDeviceManager;
class DataSource;
struct DataDevice;

struct DataOffer : mw::DataOffer
{
    DataOffer(DataSource* source, DataDevice* device);

    void accept(uint32_t serial, std::experimental::optional<std::string> const& mime_type) override
    {
        (void)serial, (void)mime_type;
    }

    void receive(std::string const& mime_type, mir::Fd fd) override;

    void destroy() override;

    void finish() override
    {
    }

    void set_actions(uint32_t dnd_actions, uint32_t preferred_action) override
    {
        (void)dnd_actions, (void)preferred_action;
    }

    void offer(std::string const& mime_type);

    DataSource* const source;
};

struct DataSource : mw::DataSource
{
public:
    DataSource(wl_resource* new_resource, DataDeviceManager* manager)
        : mw::DataSource{new_resource, Version<3>()},
          manager{manager}
    {
    }

    ~DataSource();

    void offer(std::string const& mime_type) override;

    void destroy() override;

    void set_actions(uint32_t dnd_actions) override
    {
        (void)dnd_actions;
    }

    void send_cancelled() const
    {
        if (!destroyed)
            send_cancelled_event();
    }

    void add_listener(DataOffer* listener);
    void remove_listener(DataOffer* listener);

    bool destroyed = false;
    DataDeviceManager* const manager; // Actually, this probably needs to be a listener list?
    std::vector<DataOffer*> listeners;
    std::vector<std::string> mime_types;

    void send_send(std::string const& mime_type, mir::Fd fd);
};

struct DataDevice : mw::DataDevice, mf::WlSeat::ListenerTracker
{
    DataDevice(wl_resource* new_resource, DataDeviceManager* manager, mf::WlSeat* seat);
    ~DataDevice();

    void start_drag(
        std::experimental::optional<struct wl_resource*> const& source, struct wl_resource* origin,
        std::experimental::optional<struct wl_resource*> const& icon, uint32_t serial) override
    {
        (void)source, (void)origin, (void)icon, (void)serial;
    }

    void set_selection(std::experimental::optional<struct wl_resource*> const& source, uint32_t serial) override
    {
        (void)source, (void)serial;
    }

    void release() override;

    void notify_new(DataSource* source);
    void notify_destroyed(DataSource* source);

private:
    void focus_on(wl_client *client) override;

public:

    DataDeviceManager* const manager;
    mf::WlSeat* const seat;
    bool has_focus = false;
    DataSource* current_source = nullptr;
    DataOffer* current_offer = nullptr;
};

class DataDeviceManager : public mf::DataDeviceManager
{
public:
    DataDeviceManager(struct wl_display* display);
    ~DataDeviceManager();

    void notify_destroyed(DataSource* source);

    void add_listener(DataDevice* listener);
    void remove_listener(DataDevice* listener);

private:
    using ds_ptr = std::unique_ptr<DataSource, void(*)(DataSource*)>;
    ds_ptr current_data_source;
    std::vector<DataDevice*> listeners;

    void bind(wl_resource* new_resource) override;

    class Instance : mir::wayland::DataDeviceManager
    {
    public:
        Instance(wl_resource* new_resource, ::DataDeviceManager* manager);

    private:
        void create_data_source(wl_resource* new_resource) override;
        void get_data_device(wl_resource* new_data_device, wl_resource* seat) override;

        ::DataDeviceManager* const manager;
    };
};
}

void DataSource::offer(std::string const& mime_type)
{
    mime_types.push_back(mime_type);
    for (auto const& listener : listeners)
        listener->offer(mime_type);
}

void DataSource::destroy()
{
    destroyed = true;
    manager->notify_destroyed(this);
    destroy_wayland_object();
}

void DataSource::add_listener(DataOffer* listener)
{
    listeners.push_back(listener);
}

void DataSource::remove_listener(DataOffer* listener)
{
    listeners.erase(remove(begin(listeners), end(listeners), listener), end(listeners));
}

void DataSource::send_send(std::string const& mime_type, mir::Fd fd)
{
    send_send_event(mime_type.c_str(), fd);
}

DataSource::~DataSource()
{
    if (!destroyed)
        manager->notify_destroyed(this);
}

DataDeviceManager::DataDeviceManager(struct wl_display* display) :
    mf::DataDeviceManager(display, Version<3>()),
    current_data_source{nullptr, [](DataSource* ds) { if(ds) ds->send_cancelled(); }}
{
}

DataDeviceManager::~DataDeviceManager()
{
    current_data_source.release();
}

DataDeviceManager::Instance::Instance(wl_resource* new_resource, ::DataDeviceManager* manager)
    : mir::wayland::DataDeviceManager(new_resource, Version<3>()),
      manager{manager}
{
}

void DataDeviceManager::Instance::create_data_source(wl_resource* new_data_source)
{
    manager->current_data_source.reset(new DataSource{new_data_source, manager});

    for (auto const& listener : manager->listeners)
        listener->notify_new(manager->current_data_source.get());
}

void DataDeviceManager::Instance::get_data_device(wl_resource* new_data_device, wl_resource* seat)
{
    auto const realseat = mf::WlSeat::from(seat);
    auto const result = new DataDevice{new_data_device, manager, realseat};

    if (manager->current_data_source)
        result->notify_new(manager->current_data_source.get());
}

void DataDeviceManager::notify_destroyed(DataSource* source)
{
    for (auto const& listener : listeners)
        listener->notify_destroyed(source);

    if (current_data_source.get() == source)
        current_data_source.reset();
}

void DataDeviceManager::add_listener(DataDevice* listener)
{
    listeners.push_back(listener);
}

void DataDeviceManager::remove_listener(DataDevice* listener)
{
    listeners.erase(remove(begin(listeners), end(listeners), listener), end(listeners));
}

void DataDeviceManager::bind(wl_resource* new_resource)
{
    new DataDeviceManager::Instance{new_resource, this};
}

DataDevice::DataDevice(wl_resource* new_resource, DataDeviceManager* manager, mf::WlSeat* seat) :
    mw::DataDevice(new_resource, Version<3>()),
    manager{manager},
    seat{seat}
{
    seat->add_focus_listener(this);
    manager->add_listener(this);
}

DataDevice::~DataDevice()
{
    manager->remove_listener(this);
    seat->remove_focus_listener(this);
}

void DataDevice::notify_new(DataSource* source)
{
    if (current_source)
    {
        notify_destroyed(current_source);
    }

    current_source = source;

    if (has_focus)
    {
        current_offer = new DataOffer{source, this};
    }
}

void DataDevice::notify_destroyed(DataSource* source)
{
    if (source == current_source)
    {
        current_source = nullptr;
        current_offer = nullptr;
        send_selection_event(std::experimental::nullopt);
    }
}

void DataDevice::release()
{
    manager->remove_listener(this);
    seat->remove_focus_listener(this);
    destroy_wayland_object();
}

void DataDevice::focus_on(wl_client* focus)
{
    has_focus = client == focus;

    if (has_focus && current_source && !current_offer)
    {
        current_offer = new DataOffer{current_source, this};
    }
}

DataOffer::DataOffer(DataSource* source, DataDevice* device) :
    mw::DataOffer(*device),
    source{source}
{
    source->add_listener(this);
    device->send_data_offer_event(resource);
    for (auto const& type : source->mime_types)
    {
        send_offer_event(type);
    }
    device->send_selection_event(resource);
}

void DataOffer::offer(std::string const& mime_type)
{
    send_offer_event(mime_type);
}

void DataOffer::receive(std::string const& mime_type, mir::Fd fd)
{
    source->send_send(mime_type, fd);
}

void DataOffer::destroy()
{
    source->remove_listener(this);
    destroy_wayland_object();
}

auto mf::create_data_device_manager(struct wl_display* display)
-> std::unique_ptr<DataDeviceManager>
{
    return std::unique_ptr<DataDeviceManager>{new ::DataDeviceManager(display)};
}
