/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/run_mir.h"
#include "mir/terminate_with_current_exception.h"
#include "mir/display_server.h"
#include "mir/fatal.h"
#include "mir/main_loop.h"
#include "mir/server_configuration.h"
#include "mir/frontend/connector.h"
#include "mir/raii.h"
#include "mir/emergency_cleanup.h"
#include "mir/system_executor.h"

#include <atomic>
#include <mutex>
#include <csignal>
#include <cassert>
#include <boost/throw_exception.hpp>

namespace
{
constexpr std::array const fatal_error_signals = {SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS };

std::weak_ptr<mir::EmergencyCleanup> weak_emergency_cleanup;

extern "C" void perform_emergency_cleanup()
{
    if (auto emergency_cleanup = weak_emergency_cleanup.lock())
    {
        weak_emergency_cleanup.reset();
        (*emergency_cleanup)();
    }
}

/**
 * Manage the thread- and signal-safe storage of sigaction signal handler descriptors
 */
class SignalHandlers
{
public:
    /* Read-only accessor
     * Throws if asked for a signal outside of the intercepted array, returns nullptr if handler hasn't yet
     * been initialised.
     */
    [[nodiscard]]
    auto at(int sig) const -> struct sigaction const*
    {
        for (size_t i = 0; i < handlers.size(); ++i)
        {
            if (sig == intercepted[i])
            {
                return handlers[i];
            }
        }
        mir::fatal_error_abort("Attempted to access signal handler for signal %i not in intercepted list", sig);
    }

    /**
     * Helper struct to atomically initialise an atomic<struct sigaction*> from an API using an out-pointer
     * (like, for example, sigaction())
     */
    class AtomicOutPtr
    {
    public:
        ~AtomicOutPtr() noexcept
        {
            auto old_value = to_initialise.exchange(new_value);
            delete old_value;
        }

        operator struct sigaction*()    // NOLINT: The whole point is this transparently decays to a struct sigaction*
        {
            return new_value;
        }
    private:
        friend class SignalHandlers;

        explicit AtomicOutPtr(std::atomic<struct sigaction*>& to_initialise)
            : to_initialise{to_initialise},
              new_value{new struct sigaction}
        {
        }

        std::atomic<struct sigaction*>& to_initialise;
        struct sigaction* const new_value;
    };

    /* Atomically store a sigaction descriptor.
     *
     * Because the sigaction() API initialises an out-pointer, this returns a helper to atomically
     * initialise our stored value.
     */
    auto initialiser_for_signal(int sig) -> AtomicOutPtr
    {
        for (size_t i = 0; i < handlers.size(); ++i)
        {
            if (intercepted[i] == sig)
            {
                return AtomicOutPtr{handlers[i]};
            }
        }
        mir::fatal_error_abort("Attempted to access signal handler for signal %i not in intercepted list", sig);
    }

private:
    static constexpr std::array const intercepted = { SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS };
    /* We *might* be able to get away with just a volatile array, but we *know* that atomic provides the necessary
     * guarantees
     */
    std::array<std::atomic<struct sigaction*>, intercepted.size()> handlers = { nullptr };
} old_handlers;

auto signum_to_string(int sig) -> std::string
{
    switch(sig)
    {
        case SIGQUIT:
            return "SIGQUIT";
        case SIGABRT:
            return "SIGABRT";
        case SIGFPE:
            return "SIGFPE";
        case SIGSEGV:
            return "SIGSEGV";
        case SIGBUS:
            return "SIGBUS";
        default:
            return std::string{"(Unknown signal: "} + std::to_string(sig) + ")";
    }
}

extern "C" [[noreturn]] void fatal_signal_cleanup(int sig, siginfo_t* info, void* ucontext)
{
    perform_emergency_cleanup();

    auto const old_handler = old_handlers.at(sig);
    sigaction(sig, old_handler, nullptr);
    if (old_handler->sa_flags & SA_SIGINFO)
    {
        /* The old handler wants the information from the siginfo struct
         * Re-raising the signal will destroy this context, so call the handler directly.
         */
        (*old_handler->sa_sigaction)(sig, info, ucontext);
        /* We don't *expect* to get here - fatal signal handlers will *generally* re-raise the signal,
         * and so don't return.
         *
         * However, we've just performed emergency cleanup, so if the handler was trying to fix things up
         * and continue we unfortunately can't let them.
         */
        mir::fatal_error_abort("Unsupported attempt to continue after a fatal signal: %s", signum_to_string(sig).c_str());
    }
    // The handler doesn't care about fancy context, we can just call it via raise()
    raise(sig);
    // We definitely can't continue, though, even if their handler tries.
    mir::fatal_error_abort("Unsupported attempt to continue after a fatal signal: %s", signum_to_string(sig).c_str());
}

std::atomic<struct sigaction*> wayland_sigbus_handler = nullptr;
}

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    run_mir(config, init, {});
}

void mir::run_mir(
    ServerConfiguration& config,
    std::function<void(DisplayServer&)> init,
    std::function<void(int)> const& terminator_)
{
    DisplayServer* server_ptr{nullptr};
    clear_termination_exception();

    auto const main_loop = config.the_main_loop();

    auto const& terminator = terminator_ ?
        terminator_ :
        [&server_ptr](int)
        {
          assert(server_ptr);
          server_ptr->stop();
        };

    struct sigaction old_action;
    if ((sigaction(SIGHUP, nullptr, &old_action) == 0) && (old_action.sa_handler == SIG_IGN))
    {
        // If our parent process is ignoring SIGHUP (e.g. is nohup) then we do the same
        main_loop->register_signal_handler({SIGINT, SIGTERM}, terminator);
    }
    else
    {
        main_loop->register_signal_handler({SIGINT, SIGHUP, SIGTERM}, terminator);
    }

    FatalErrorStrategy fatal_error_strategy{config.the_fatal_error_strategy()};

    DisplayServer server(config);
    server_ptr = &server;

    weak_emergency_cleanup = config.the_emergency_cleanup();

    static std::atomic<unsigned int> concurrent_calls{0};

    auto const raii = raii::paired_calls(
        [&]()
        {
            if (!concurrent_calls++)
            {
                for (auto sig : fatal_error_signals)
                {
                    struct sigaction sig_handler_desc;
                    sig_handler_desc.sa_flags = SA_SIGINFO;
                    sig_handler_desc.sa_sigaction = &fatal_signal_cleanup;

                    if (sigaction(sig, &sig_handler_desc, old_handlers.initialiser_for_signal(sig)))
                    {
                        using namespace std::string_literals;
                        BOOST_THROW_EXCEPTION((
                            std::system_error{
                                errno,
                                std::system_category(),
                                "Failed to install signal handler for "s + signum_to_string(sig)
                            }));
                    }
                }
                /* We need to handle SIGBUS specially. Particularly: libwayland installs a SIGBUS handler
                 * the *first* time a process calls wl_shm_buffer_begin_access; this will be after we've
                 * installed our own handlers, so we haven't saved it.
                 *
                 * However, in order to make it possible to call run_mir() twice in the same process, we need to
                 * be able to call libwayland's SIGBUS handler, so that out-of-bounds accesses to wl_shm_buffers
                 * can result in protocol errors rather than terminating Mir.
                 *
                 * Check if we have previously captured libwayland's SIGBUS handler, and install that instead.
                 *
                 * This can only happen after the first call to run_mir() in this process has finished.
                 *
                 * In that case, libwayland will have captured *our* SIGBUS handler, and will call it if
                 * it's handler fails, so re-installing libwayland's handler will do what we want
                 */
                if (wayland_sigbus_handler)
                {
                    // We don't need to capture the old handler, because we know it's the one we just set above.
                    if (sigaction(SIGBUS, wayland_sigbus_handler, nullptr))
                    {
                        BOOST_THROW_EXCEPTION((
                            std::system_error{
                                errno,
                                std::system_category(),
                                "Failed to install signal handler for SIGBUS"
                            }));
                    }
                }
            }
        },
        [&]()
        {
            if (!--concurrent_calls)
            {
                for (auto sig : fatal_error_signals)
                {
                    if (sig == SIGBUS)
                    {
                        /* Dance the signal handler fandango!
                         *
                         * *If* libwayland has installed its SIGBUS handler, then we need to capture it so that
                         * we can restore it on the next run_mir() invocation, because libwayland will only install
                         * its handler once per process.
                         *
                         * libwayland may not have installed a SIGBUS handler; it only does so on the first call
                         * to wl_shm_buffer_begin_access(). Detect this case by comparing the currently-installed
                         * SIGBUS handler with the one we installed. If it's not our handler, assume that it's
                         * the one that libwayland has installed.
                         *
                         * Because signal handlers are a terrifying bundle of global mutable state, we can't guarantee
                         * that this *is* libwayland's SIGBUS handler, but if the code using Mir is trying to install
                         * its own SIGBUS handler after us that's on them.
                         */
                        struct sigaction current_handler;
                        if (sigaction(sig, nullptr, &current_handler))
                        {
                            BOOST_THROW_EXCEPTION((
                                std::system_error{
                                    errno,
                                    std::system_category(),
                                    "Failed to query current SIGBUS handler"
                                }));
                        }
                        if (current_handler.sa_sigaction != &fatal_signal_cleanup)
                        {
                            /* Our signal handler has been replaced (presumably by libwayland)
                             * Store it so we can restore it on the next run_mir() call
                             */
                            auto new_handler = new struct sigaction;
                            *new_handler = current_handler;
                            auto old_handler = wayland_sigbus_handler.exchange(new_handler);
                            delete old_handler;
                        }
                        // Now go on and restore the previous (to us) SIGBUS handler...
                    }
                    if (sigaction(sig, old_handlers.at(sig), nullptr))
                    {
                        using namespace std::string_literals;
                        BOOST_THROW_EXCEPTION((
                            std::system_error{
                                errno,
                                std::system_category(),
                                "Failed to install signal handler for "s + signum_to_string(sig)
                            }));
                    }
                }
            }
        });

    mir::SystemExecutor::set_unhandled_exception_handler(&terminate_with_current_exception);

    init(server);
    server.run();

    mir::SystemExecutor::quiesce();
    check_for_termination_exception();
}
