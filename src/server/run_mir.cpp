/*
 * Copyright © Canonical Ltd.
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
#include "mir/executor.h"

#include <atomic>
#include <mutex>
#include <csignal>
#include <unistd.h>
#include <boost/throw_exception.hpp>
#include <errno.h>

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
    {
        /* Oh. Oh no.
         *
         * We've received a fatal signal. Time to crash!
         *
         * The rest of this handler consists almost entierly of *wildly* un-signal-safe
         * calls, and since it calls code distributed throughout Mir it is unreasonable
         * to try to make this signal-safe.
         *
         * Since we're crashing *anyway*, start with a log that *is* signal-safe, and
         * then cross our fingers for the rest.
         */
        constexpr char const* warning = "!!! Fatal signal received. Attempting cleanup, but deadlock may occur\n";
        constexpr size_t len = std::char_traits<char>::length(warning);
        [[maybe_unused]]auto n = write(STDERR_FILENO, warning, len);
    }

    // Log a security event
    char security_head[] = "{\"datetime\": \"YYYY-MM-DDThh:mm:ssZ";
    char security_mid[]  = "\", \"appid\": \"";
    char security_tail[] = "\", \"event\": \"crash\", \"description\": \"Fatal signal received\"}\n";
    std::time_t time = std::time(nullptr);
    // strftime isn't listed as async-signal-safe, but neither "%F" or "%T" use locale information
    [[maybe_unused]]auto n = std::strftime(security_head + 14, 21, "%FT%TZ", std::gmtime(&time));
    n = write(STDERR_FILENO, security_head, sizeof(security_head) - 1);
    n = write(STDERR_FILENO, security_mid, sizeof(security_mid) - 1);
    if (program_invocation_short_name)
    {
        n = write(STDERR_FILENO, program_invocation_short_name, std::strlen(program_invocation_short_name));
    }
    else
    {
        n = write(STDERR_FILENO, "<unknown>", 9);
    }
    n = write(STDERR_FILENO, security_tail, sizeof(security_tail) - 1);

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
            if (!server_ptr)
                fatal_error_abort("Logic error in mir::run_mir() server_ptr is nullptr");
            server_ptr->stop();
        };

    struct sigaction old_action{};
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
                    struct sigaction sig_handler_desc{};
                    sigfillset(&sig_handler_desc.sa_mask);
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
            }
        },
        [&]()
        {
            if (!--concurrent_calls)
            {
                for (auto sig : fatal_error_signals)
                {
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

    mir::ThreadPoolExecutor::set_unhandled_exception_handler(&terminate_with_current_exception);

    init(server);
    server.run();

    mir::ThreadPoolExecutor::quiesce();
    check_for_termination_exception();
}
