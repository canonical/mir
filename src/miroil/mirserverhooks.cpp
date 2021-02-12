/*
 * Copyright © 2016 Canonical Ltd.
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

#include "miroil/mirserverhooks.h"
#include "miroil/namedcursor.h"
#include <stdexcept>

// mir
#include <mir/scene/prompt_session_listener.h>
#include <mir/server.h>
#include <mir/input/input_device_hub.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir/input/input_device_observer.h>
#include <mir/graphics/cursor.h>
#include <mir/input/cursor_images.h>

namespace mg = mir::graphics;
namespace ms = mir::scene;
    
struct PromptSessionListenerImpl : mir::scene::PromptSessionListener
{
    PromptSessionListenerImpl(std::shared_ptr<miroil::PromptSessionListener> const& listener) : listener(listener) {};
    ~PromptSessionListenerImpl();
    
    miroil::PromptSessionListener * promptSessionListener() const { return listener.get(); };
    
    void starting(std::shared_ptr<mir::scene::PromptSession> const& prompt_session) override;
    void stopping(std::shared_ptr<mir::scene::PromptSession> const& prompt_session) override;
    void suspending(std::shared_ptr<mir::scene::PromptSession> const& prompt_session) override;
    void resuming(std::shared_ptr<mir::scene::PromptSession> const& prompt_session) override;

    void prompt_provider_added(mir::scene::PromptSession const& prompt_session,
                               std::shared_ptr<mir::scene::Session> const& prompt_provider) override;
    void prompt_provider_removed(mir::scene::PromptSession const& prompt_session,
                                 std::shared_ptr<mir::scene::Session> const& prompt_provider) override;
private:    
    std::shared_ptr<miroil::PromptSessionListener> const listener;
};

struct MirInputDeviceObserverImpl : mir::input::InputDeviceObserver
{
public:    
    MirInputDeviceObserverImpl(std::shared_ptr<miroil::InputDeviceObserver> & observer) : observer(observer) {};
    
    void device_added(std::shared_ptr<mir::input::Device> const& device) override;
    void device_changed(std::shared_ptr<mir::input::Device> const& /*device*/) override {}
    void device_removed(std::shared_ptr<mir::input::Device> const& device) override;
    void changes_complete() override {}
    
private:    
    std::shared_ptr<miroil::InputDeviceObserver> observer;
};

struct HiddenCursorWrapper : mg::Cursor
{
    HiddenCursorWrapper(std::shared_ptr<mg::Cursor> const& wrapped) :
        wrapped{wrapped} { wrapped->hide(); }
    //void show() override { }
    void show(mg::CursorImage const&) override { }
    void hide() override { wrapped->hide(); }

    void move_to(mir::geometry::Point position) override { wrapped->move_to(position); }

private:
    std::shared_ptr<mg::Cursor> const wrapped;
};

class MirCursorImages : public mir::input::CursorImages
{
public:
    std::shared_ptr<mir::graphics::CursorImage> image(const std::string &cursor_name,
            const mir::geometry::Size &size) override;
};

std::shared_ptr<mir::graphics::CursorImage> MirCursorImages::image(const std::string &cursor_name,
        const mir::geometry::Size &)
{
    // We are not responsible for loading cursors. This is left for shell to do as it's drawing its own QML cursor.
    // So here we work around Mir API by storing just the cursor name in the CursorImage.
    return std::make_shared<miroil::NamedCursor>(cursor_name.c_str());
}

struct miroil::MirServerHooks::Self
{
    std::function<std::shared_ptr<miroil::PromptSessionListener>()> createListener;
    std::weak_ptr<PromptSessionListenerImpl> m_promptSessionListener;
    std::weak_ptr<mir::graphics::Display> m_mirDisplay;
    std::weak_ptr<mir::shell::DisplayConfigurationController> m_mirDisplayConfigurationController;
    std::weak_ptr<mir::scene::PromptSessionManager> m_mirPromptSessionManager;
    std::weak_ptr<mir::input::InputDeviceHub> m_inputDeviceHub;
};

miroil::MirServerHooks::MirServerHooks(std::function<std::shared_ptr<miroil::PromptSessionListener>()> createListener) :
    self{std::make_shared<Self>()}
{
    self->createListener = createListener;
}

void miroil::MirServerHooks::operator()(mir::Server& server)
{
    server.override_the_cursor_images([]
        { return std::make_shared<MirCursorImages>(); });

    server.wrap_cursor([&](std::shared_ptr<mg::Cursor> const& wrapped)
        { return std::make_shared<HiddenCursorWrapper>(wrapped); });
    
    if (self->createListener) {
        server.override_the_prompt_session_listener([this]
        {            
            auto promptListener = self->createListener();
            auto const result = std::make_shared<PromptSessionListenerImpl>(promptListener);
            self->m_promptSessionListener = result;
            return result;
        });
    }

    server.add_init_callback([this, &server]
        {
            self->m_mirDisplay = server.the_display();
            self->m_mirDisplayConfigurationController = server.the_display_configuration_controller();
            self->m_mirPromptSessionManager = server.the_prompt_session_manager();
            self->m_inputDeviceHub = server.the_input_device_hub();
        });
}

miroil::PromptSessionListener * miroil::MirServerHooks::promptSessionListener() const
{
    if (auto result = self->m_promptSessionListener.lock()) {        
        return result.get()->promptSessionListener();
    }    

    throw std::logic_error("No prompt session listener available. Server not running?");
}

std::shared_ptr<mir::scene::PromptSessionManager> miroil::MirServerHooks::thePromptSessionManager() const
{
    if (auto result = self->m_mirPromptSessionManager.lock())
        return result;

    throw std::logic_error("No prompt session manager available. Server not running?");
}

std::shared_ptr<mir::graphics::Display> miroil::MirServerHooks::theMirDisplay() const
{
    if (auto result = self->m_mirDisplay.lock())
        return result;

    throw std::logic_error("No display available. Server not running?");
}

std::shared_ptr<mir::input::InputDeviceHub> miroil::MirServerHooks::theInputDeviceHub() const
{
    if (auto result = self->m_inputDeviceHub.lock())
        return result;

    throw std::logic_error("No input device hub available. Server not running?");
}

std::shared_ptr<mir::shell::DisplayConfigurationController> miroil::MirServerHooks::theDisplayConfigurationController() const
{
    if (auto result = self->m_mirDisplayConfigurationController.lock())
        return result;

    throw std::logic_error("No input device hub available. Server not running?");
}

void miroil::MirServerHooks::createInputDeviceObserver(std::shared_ptr<miroil::InputDeviceObserver> & observer)
{
    theInputDeviceHub()->add_observer(std::make_shared<MirInputDeviceObserverImpl>(observer));
}

PromptSessionListenerImpl::~PromptSessionListenerImpl() = default;

void PromptSessionListenerImpl::starting(std::shared_ptr<mir::scene::PromptSession> const& prompt_session)
{
    listener->starting(prompt_session);
}

void PromptSessionListenerImpl::stopping(std::shared_ptr<mir::scene::PromptSession> const& prompt_session)
{
    listener->stopping(prompt_session);
}

void PromptSessionListenerImpl::suspending(std::shared_ptr<mir::scene::PromptSession> const& prompt_session)
{
    listener->suspending(prompt_session);
}

void PromptSessionListenerImpl::resuming(std::shared_ptr<mir::scene::PromptSession> const& prompt_session)
{
    listener->resuming(prompt_session);
}

void PromptSessionListenerImpl::prompt_provider_added(mir::scene::PromptSession const& prompt_session,
                                                      std::shared_ptr<ms::Session> const& prompt_provider)
{
    listener->prompt_provider_added(prompt_session, prompt_provider);
}

void PromptSessionListenerImpl::prompt_provider_removed(mir::scene::PromptSession const& prompt_session,
                                                        std::shared_ptr<ms::Session> const& prompt_provider)
{
    listener->prompt_provider_removed(prompt_session, prompt_provider);
}

void MirInputDeviceObserverImpl::device_added(const std::shared_ptr<mir::input::Device> &device)
{
    observer->device_added(device);
}

void MirInputDeviceObserverImpl::device_removed(const std::shared_ptr<mir::input::Device> &device)
{
    if (device) {
        observer->device_removed(device);
    }
}

