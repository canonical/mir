/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "default_ipc_factory.h"

#include "no_prompt_shell.h"
#include "session_mediator.h"
#include "authorizing_display_changer.h"
#include "authorizing_input_config_changer.h"
#include "unauthorized_screencast.h"
#include "resource_cache.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/frontend/event_sink.h"
#include "event_sink_factory.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/cookie/authority.h"
#include "mir/executor.h"
#include "mir/signal_blocker.h"

#include <deque>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;

namespace
{
class ThreadExecutor : public mir::Executor
{
public:
    ThreadExecutor() = default;

    ThreadExecutor(ThreadExecutor const&) = delete;
    ThreadExecutor& operator=(ThreadExecutor const&) = delete;

    void do_work() noexcept
    {
        std::unique_lock<std::mutex> lock{queue_mutex};
        for(;;)
        {
            while (!tasks.empty())
            {
                std::function<void()> task;
                task = std::move(tasks.front());
                tasks.pop_front();

                lock.unlock();
                task();
                /*
                 * The task functor may have captured resources with non-trivial
                 * destructors.
                 *
                 * Ensure those destructors are called outside the lock.
                 */
                task = nullptr;
                lock.lock();
            }

            if (state != State::Running)
            {
                return;
            }

            queue_notifier.wait(
                lock,
                [this]()
                {
                    return (state != State::Running) || !tasks.empty();
                });
        }
    }

    ~ThreadExecutor()
    {
        quiesce();
    }

    void spawn(std::function<void()>&& work) override
    {
        {
            std::lock_guard<std::mutex> lock{queue_mutex};
            tasks.emplace_back(std::move(work));

            if (state == State::NotYetStarted)
            {
                /*
                 * Block all signals on the dispatch thread.
                 *
                 * Threads inherit their parent's signal mask, so use a SignalBlocker to block
                 * all signals *before* spawning the thread (and then restore the signal mask
                 * when this constructor completes).
                 */
                mir::SignalBlocker blocker;
                state = State::Running;
                dispatch_thread = std::thread{std::bind(&ThreadExecutor::do_work, this)};
            }
        }
        queue_notifier.notify_all();
    }

    void quiesce()
    {
        {
            std::lock_guard<std::mutex> lock{queue_mutex};
            state = State::Quiesced;
        }
        queue_notifier.notify_all();

        if (dispatch_thread.joinable())
            dispatch_thread.join();
    }

    void resume()
    {
        std::lock_guard<std::mutex> lock{queue_mutex};
        state = State::NotYetStarted;
        if (!tasks.empty())
        {
            /*
             * Block all signals on the dispatch thread.
             *
             * Threads inherit their parent's signal mask, so use a SignalBlocker to block
             * all signals *before* spawning the thread (and then restore the signal mask
             * when this constructor completes).
             */
            mir::SignalBlocker blocker;
            state = State::Running;
            dispatch_thread = std::thread{std::bind(&ThreadExecutor::do_work, this)};
        }
    }

    void discard()
    {
        std::lock_guard<std::mutex> lock{queue_mutex};
        tasks.clear();
        state = State::NotYetStarted;
    }
private:
    std::thread dispatch_thread;

    std::mutex queue_mutex;
    std::condition_variable queue_notifier;
    enum class State
    {
        NotYetStarted,
        Running,
        Quiesced
    } state{State::NotYetStarted};
    std::deque<std::function<void()>> tasks;
};

mir::Executor& buffer_return_ipc_executor()
{
    static std::once_flag setup;
    static ThreadExecutor executor;

    std::call_once(
        setup,
        []()
        {
            /*
             * fork() interacts with threads by screaming about being
             * smothered by moths and then gibbering in a corner.
             *
             * Conveniently, our test-suite makes extensive use of fork(),
             * and runs all the tests from a single main process, meaning
             * that once a single test has called executor.spawn() every
             * subsequent call to fork() is a call from a multithreaded
             * program.
             *
             * Enter the moths.
             *
             * We can get around this by quiescing the executor; temporarily
             * halting its execution thread and then resuming it post-fork.
             */
            pthread_atfork(
                []()
                {
                    // Pre-fork
                    executor.quiesce();
                },
                []()
                {
                    /*
                     * Post-fork, in the parent:
                     * Resume execution, executing any functors queued
                     * since quiescence.
                     */
                    executor.resume();
                },
                []()
                {
                    /*
                     * Post-fork, in the child:
                     * Discard any tasks that snuck in after pre-fork but before fork();
                     * they'll be handled in the parent.
                     */
                    executor.discard();
                });
        });

    return executor;
}
}

mf::DefaultIpcFactory::DefaultIpcFactory(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<SessionMediatorObserver> const& sm_observer,
    std::shared_ptr<mg::PlatformIpcOperations> const& platform_ipc_operations,
    std::shared_ptr<DisplayChanger> const& display_changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Screencast> const& screencast,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer,
    std::shared_ptr<mi::CursorImages> const& cursor_images,
    std::shared_ptr<scene::CoordinateTranslator> const& translator,
    std::shared_ptr<scene::ApplicationNotRespondingDetector> const& anr_detector,
    std::shared_ptr<mir::cookie::Authority> const& cookie_authority,
    std::shared_ptr<InputConfigurationChanger> const& input_changer,
    std::vector<mir::ExtensionDescription> const& extensions) :
    shell(shell),
    no_prompt_shell(std::make_shared<NoPromptShell>(shell)),
    sm_observer(sm_observer),
    cache(std::make_shared<ResourceCache>()),
    platform_ipc_operations(platform_ipc_operations),
    display_changer(display_changer),
    buffer_allocator(buffer_allocator),
    screencast(screencast),
    session_authorizer(session_authorizer),
    cursor_images(cursor_images),
    translator{translator},
    anr_detector{anr_detector},
    cookie_authority(cookie_authority),
    input_changer(input_changer),
    extensions(extensions)
{
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_ipc_server(
    SessionCredentials const &creds,
    std::shared_ptr<EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    ConnectionContext const &connection_context)
{
    bool input_configuration_is_authorized = session_authorizer->configure_input_is_allowed(creds);
    bool base_input_configuration_is_authorized = session_authorizer->set_base_input_configuration_is_allowed(creds);
    auto const input_config_changer = std::make_shared<AuthorizingInputConfigChanger>(
        input_changer,
        input_configuration_is_authorized,
        base_input_configuration_is_authorized);

    bool configuration_is_authorized = session_authorizer->configure_display_is_allowed(creds);
    bool base_configuration_is_authorized = session_authorizer->set_base_display_configuration_is_allowed(creds);

    auto const changer = std::make_shared<AuthorizingDisplayChanger>(
        display_changer,
        configuration_is_authorized,
        base_configuration_is_authorized);
    std::shared_ptr<Screencast> effective_screencast;

    if (session_authorizer->screencast_is_allowed(creds))
    {
        effective_screencast = screencast;
    }
    else
    {
        effective_screencast = std::make_shared<UnauthorizedScreencast>();
    }

    auto const allow_prompt_session =
        session_authorizer->prompt_session_is_allowed(creds);

    auto const effective_shell = allow_prompt_session ? shell : no_prompt_shell;

    return make_mediator(
        effective_shell,
        platform_ipc_operations,
        changer,
        buffer_allocator,
        sm_observer,
        sink_factory,
        message_sender,
        effective_screencast,
        connection_context,
        cursor_images,
        input_config_changer);
}

std::shared_ptr<mf::ResourceCache> mf::DefaultIpcFactory::resource_cache()
{
    return cache;
}

std::shared_ptr<mf::detail::DisplayServer> mf::DefaultIpcFactory::make_mediator(
    std::shared_ptr<Shell> const& shell,
    std::shared_ptr<mg::PlatformIpcOperations> const& platform_ipc_operations,
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<SessionMediatorObserver> const& sm_observer,
    std::shared_ptr<mf::EventSinkFactory> const& sink_factory,
    std::shared_ptr<mf::MessageSender> const& message_sender,
    std::shared_ptr<Screencast> const& effective_screencast,
    ConnectionContext const& connection_context,
    std::shared_ptr<mi::CursorImages> const& cursor_images,
    std::shared_ptr<InputConfigurationChanger> const& input_changer)
{
    return std::make_shared<SessionMediator>(
        shell,
        platform_ipc_operations,
        changer,
        buffer_allocator->supported_pixel_formats(),
        sm_observer,
        sink_factory,
        message_sender,
        resource_cache(),
        effective_screencast,
        connection_context,
        cursor_images,
        translator,
        anr_detector,
        cookie_authority,
        input_changer,
        extensions,
        buffer_allocator,
        buffer_return_ipc_executor());
}
