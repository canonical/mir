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

namespace mf = mir::frontend;

namespace
{
class DataDeviceManager;
class DataSource;

struct DataOffer : mf::wayland::DataOffer
{
    DataOffer(struct wl_client* client, struct wl_resource* parent, uint32_t id, DataSource* source);

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

struct DataSource : mf::wayland::DataSource
{
    DataSource(struct wl_client* client, struct wl_resource* parent, uint32_t id, DataDeviceManager* manager) :
        mf::wayland::DataSource{client, parent, id},
        manager{manager}
    {
    }

    void offer(std::string const& mime_type) override;

    void destroy() override;

    void set_actions(uint32_t dnd_actions) override
    {
        (void)dnd_actions;
    }

    void send_cancelled() const
    {
        if (!destroyed)
            wl_data_source_send_cancelled(resource);
    }

    void add_listener(DataOffer* listener);
    void remove_listener(DataOffer* listener);

    bool destroyed = false;
    DataDeviceManager* const manager; // Actually, this probably needs to be a listener list?
    std::vector<DataOffer*> listeners;
    std::vector<std::string> mime_types;

    void send_send(std::string const& mime_type, mir::Fd fd);
};

struct DataDevice : mf::wayland::DataDevice, mf::WlSeat::ListenerTracker
{
    DataDevice(struct wl_client* client, struct wl_resource* parent, uint32_t id, DataDeviceManager* manager, mf::WlSeat* seat);
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

    void create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id) override;

    void get_data_device(
        struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat) override;

    void notify_destroyed(DataSource* source);

    void add_listener(DataDevice* listener);
    void remove_listener(DataDevice* listener);

    private:
    using ds_ptr = std::unique_ptr<DataSource, void(*)(DataSource*)>;
    ds_ptr current_data_source;
    std::vector<DataDevice*> listeners;
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
    wl_resource_destroy(resource);
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
    wl_data_source_send_send(resource, mime_type.c_str(), fd);
}

DataDeviceManager::DataDeviceManager(struct wl_display* display) :
    mf::DataDeviceManager(display, 3),
    current_data_source{nullptr, [](DataSource* ds) { if(ds) ds->send_cancelled(); }}
{
}

DataDeviceManager::~DataDeviceManager()
{
    current_data_source.release();
}

void DataDeviceManager::create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
    (void)client, (void)resource, (void)id;

    current_data_source.reset(new DataSource{client, resource, id, this});

    for (auto const& listener : listeners)
        listener->notify_new(current_data_source.get());
}

void DataDeviceManager::get_data_device(
    struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat)
{
    auto const realseat = mf::WlSeat::from(seat);
    auto const result = new DataDevice{client, resource, id, this, realseat};

    if (current_data_source)
        result->notify_new(current_data_source.get());
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

DataDevice::DataDevice(struct wl_client* client, struct wl_resource* parent, uint32_t id, DataDeviceManager* manager, mf::WlSeat* seat) :
    mf::wayland::DataDevice(client, parent, id),
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
        current_offer = new DataOffer{client, resource, 0, source};
    }
}

void DataDevice::notify_destroyed(DataSource* source)
{
    if (source == current_source)
    {
        current_source = nullptr;
        current_offer = nullptr;
        wl_data_device_send_selection(resource, 0);
    }
}

void DataDevice::release()
{
    manager->remove_listener(this);
    seat->remove_focus_listener(this);
    wl_resource_destroy(resource);
}

void DataDevice::focus_on(wl_client* focus)
{
    has_focus = client == focus;

    if (has_focus && current_source && !current_offer)
    {
        current_offer = new DataOffer{client, resource, 0, current_source};
    }
}

DataOffer::DataOffer(struct wl_client* client, struct wl_resource* parent, uint32_t id, DataSource* source) :
    mf::wayland::DataOffer(client, parent, id),
    source{source}
{
    source->add_listener(this);
    wl_data_device_send_data_offer(parent, resource);
    for (auto const& type : source->mime_types)
    {
        wl_data_offer_send_offer(resource, type.c_str());
    }
    wl_data_device_send_selection(parent, resource);
}

void DataOffer::offer(std::string const& mime_type)
{
    wl_data_offer_send_offer(resource, mime_type.c_str());
}

void DataOffer::receive(std::string const& mime_type, mir::Fd fd)
{
    source->send_send(mime_type, fd);
}

void DataOffer::destroy()
{
    source->remove_listener(this);
    wl_resource_destroy(resource);
}

auto mf::create_data_device_manager(struct wl_display* display)
-> std::unique_ptr<DataDeviceManager>
{
    return std::unique_ptr<DataDeviceManager>{new ::DataDeviceManager(display)};
}