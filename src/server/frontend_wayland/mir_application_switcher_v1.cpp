/*
* Copyright © Canonical Ltd.
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
 */

#include "mir_application_switcher_v1.h"
#include "mir/synchronised.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace msh = mir::shell;

namespace
{
uint32_t to_raw_state(msh::ApplicationSwitcherState state)
{
    switch (state)
    {
        case msh::ApplicationSwitcherState::all_forward:
            return mw::MirApplicationSwitcherHandleV1::State::all_forward;
        case msh::ApplicationSwitcherState::all_backward:
            return mw::MirApplicationSwitcherHandleV1::State::all_backward;
        case msh::ApplicationSwitcherState::within_forward:
            return mw::MirApplicationSwitcherHandleV1::State::within_forward;
        case msh::ApplicationSwitcherState::within_backward:
            return mw::MirApplicationSwitcherHandleV1::State::within_backward;
        default:
            throw std::logic_error("Invalid state");
    }
}

class Handle : public mw::MirApplicationSwitcherHandleV1
{
public:
    explicit Handle(wl_resource* new_resource)
        : MirApplicationSwitcherHandleV1(new_resource, Version<1>{})
    {}
};

class Instance : public mw::MirApplicationSwitcherV1
{
public:
    explicit Instance(wl_resource* new_resource, std::function<void(wl_resource*)>&& on_handle)
        : MirApplicationSwitcherV1(new_resource, Version<1>{}),
          on_handle(std::move(on_handle))
    {
    }

private:
    void create(wl_resource* id) override
    {
        on_handle(id);
    }

    std::function<void(wl_resource*)> on_handle;
};

class Global : public mw::MirApplicationSwitcherV1::Global
{
public:
    explicit Global(wl_display* display, std::shared_ptr<msh::ApplicationSwitcher> const& app_switcher)
        : mw::MirApplicationSwitcherV1::Global(display, Version<1>{}),
          app_switcher(app_switcher),
          observer(std::make_shared<Observer>(this))
    {
        app_switcher->register_interest(observer);
    }

    ~Global() override
    {
        app_switcher->unregister_interest(*observer);
    }

    void bind(wl_resource* new_mir_application_switcher_v1) override
    {
        new Instance(new_mir_application_switcher_v1, [this](auto const id)
        {
            *handle.lock() = std::make_shared<Handle>(id);
        });
    }

    mir::Synchronised<std::shared_ptr<Handle>> handle;

private:
    class Observer : public msh::ApplicationSwitcherObserver
    {
    public:
        Observer(Global* global)
            : global(global)
        {
        }

        void activate(msh::ApplicationSwitcherState state) override
        {
            global->handle.lock()->get()->send_activate_event(to_raw_state(state));
        }

        void deactivate() override
        {
            global->handle.lock()->get()->send_deactivate_event();
        }

        void set_state(msh::ApplicationSwitcherState state) override
        {
            global->handle.lock()->get()->send_state_event(to_raw_state(state));
        }

        void next() override
        {
            global->handle.lock()->get()->send_next_event();
        }

        Global* global;
    };

    std::shared_ptr<msh::ApplicationSwitcher> app_switcher;
    std::shared_ptr<Observer> observer;
};
}

auto mf::create_mir_application_switcher(
    wl_display* display,
    std::shared_ptr<msh::ApplicationSwitcher> const& app_switcher)
    -> std::shared_ptr<wayland::MirApplicationSwitcherV1::Global>
{
    return std::make_shared<Global>(display, app_switcher);
}
