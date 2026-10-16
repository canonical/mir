/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/debug_frame_trace.h>
#include <mir/log.h>

#include <GLES2/gl2.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <map>
#include <thread>
#include <unistd.h>
#include <sys/eventfd.h>

namespace md = mir::debug;

namespace
{
char const* const component = "mir-debug";

auto start_time() -> std::chrono::steady_clock::time_point const&
{
    static auto const start = std::chrono::steady_clock::now();
    return start;
}

auto parse_sync_mode() -> md::BufferSyncMode
{
    auto const* const env = std::getenv("MIR_DEBUG_BUFFER_SYNC");
    if (!env)
        return md::BufferSyncMode::off;

    struct { char const* name; md::BufferSyncMode mode; } const modes[] = {
        {"off",               md::BufferSyncMode::off},
        {"per-buffer",        md::BufferSyncMode::per_buffer},
        {"per-buffer-finish", md::BufferSyncMode::per_buffer_finish},
        {"global",            md::BufferSyncMode::global},
        {"global-finish",     md::BufferSyncMode::global_finish},
        {"reimport",          md::BufferSyncMode::reimport}
    };

    for (auto const& [name, mode] : modes)
    {
        if (std::strcmp(env, name) == 0)
        {
            mir::log(mir::logging::Severity::informational, component,
                std::string{"MIR_DEBUG_BUFFER_SYNC="} + name);
            return mode;
        }
    }

    mir::log(mir::logging::Severity::warning, component,
        std::string{"Unrecognised MIR_DEBUG_BUFFER_SYNC value: "} + env);
    return md::BufferSyncMode::off;
}

struct BufferTrace
{
    std::string format;
    uint64_t modifier{0};
    unsigned planes{0};
    int width{0};
    int height{0};
    double imported{0};
    double last_sampled{0};
    double released{0};
    uint64_t samples{0};
    std::string last_sampled_by;
};

struct State
{
    std::mutex mutex;
    std::map<std::string, md::OutputTrace> outputs;
    std::map<uint32_t, BufferTrace> buffers;
    std::atomic<uint64_t> concurrent_events{0};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> samplers_in_flight{0};
};

auto state() -> State&
{
    static State state;
    return state;
}

auto dump_fd() -> int&
{
    static int fd{-1};
    return fd;
}

extern "C" void on_sigusr2(int)
{
    uint64_t const one{1};
    // write() is async-signal-safe; the dumping happens on a normal thread
    [[maybe_unused]] auto const ignored = ::write(dump_fd(), &one, sizeof(one));
}
}

auto md::buffer_sync_mode() -> md::BufferSyncMode
{
    static auto const mode = parse_sync_mode();
    return mode;
}

auto md::frame_trace_level() -> int
{
    static auto const level =
        []
        {
            auto const* const env = std::getenv("MIR_DEBUG_FRAME_TRACE");
            auto const value = env ? std::atoi(env) : 0;
            if (value > 0)
            {
                mir::log(mir::logging::Severity::informational, component,
                    std::format("MIR_DEBUG_FRAME_TRACE={}", value));
            }
            return value;
        }();
    return level;
}

auto md::now_seconds() -> double
{
    return std::chrono::duration<double>{std::chrono::steady_clock::now() - start_time()}.count();
}

void md::trace_message(std::string_view message)
{
    mir::log(
        mir::logging::Severity::informational, component,
        std::format("[{:12.6f} tid={}] {}", now_seconds(), gettid(), message));
}

void md::trace_warning_message(std::string_view message)
{
    mir::log(
        mir::logging::Severity::warning, component,
        std::format("[{:12.6f} tid={}] {}", now_seconds(), gettid(), message));
}

void md::note_sample_begin(std::atomic<int>& counter, uint32_t buffer_id, unsigned tex_id)
{
    auto const concurrent = counter.fetch_add(1, std::memory_order_acq_rel) + 1;
    auto const in_flight = state().samplers_in_flight.fetch_add(1, std::memory_order_acq_rel) + 1;

    auto previous_max = state().max_concurrent.load(std::memory_order_relaxed);
    while (concurrent > previous_max &&
           !state().max_concurrent.compare_exchange_weak(previous_max, concurrent))
    {
    }

    if (concurrent > 1)
    {
        state().concurrent_events.fetch_add(1, std::memory_order_relaxed);
        if (tracing())
        {
            trace_warning(
                "CONCURRENT SAMPLE buf={} tex={} samplers_of_this_buffer={} samplers_in_flight={}",
                buffer_id, tex_id, concurrent, in_flight);
        }
    }
    else if (tracing_verbose())
    {
        trace("sample begin buf={} tex={} samplers_in_flight={}", buffer_id, tex_id, in_flight);
    }
}

void md::note_sample_end(std::atomic<int>& counter, uint32_t buffer_id)
{
    counter.fetch_sub(1, std::memory_order_acq_rel);
    state().samplers_in_flight.fetch_sub(1, std::memory_order_acq_rel);

    if (tracing_verbose())
        trace("sample end   buf={}", buffer_id);
}

auto md::concurrent_sample_events() -> uint64_t
{
    return state().concurrent_events.load(std::memory_order_relaxed);
}

auto md::max_concurrent_samplers() -> int
{
    return state().max_concurrent.load(std::memory_order_relaxed);
}

auto md::global_render_mutex() -> std::mutex&
{
    static std::mutex mutex;
    return mutex;
}

void md::gl_finish()
{
    glFinish();
}

void md::note_buffer_imported(
    uint32_t buffer_id,
    std::string_view format,
    uint64_t modifier,
    unsigned planes,
    int width,
    int height)
{
    if (!tracing())
        return;

    install_state_dump_handler();

    {
        std::lock_guard lock{state().mutex};
        state().buffers[buffer_id] = BufferTrace{
            std::string{format}, modifier, planes, width, height, now_seconds(), 0, 0, 0, {}};
    }

    trace(
        "buffer import buf={} format={} modifier=0x{:016x} planes={} size={}x{}",
        buffer_id, format, modifier, planes, width, height);
}

void md::note_buffer_sampled(uint32_t buffer_id, std::string_view output)
{
    if (!tracing())
        return;

    std::lock_guard lock{state().mutex};
    auto& buffer = state().buffers[buffer_id];
    ++buffer.samples;
    buffer.last_sampled = now_seconds();
    buffer.last_sampled_by = std::string{output};
}

void md::note_buffer_released(uint32_t buffer_id)
{
    if (!tracing())
        return;

    double last_sampled{0};
    uint64_t samples{0};
    {
        std::lock_guard lock{state().mutex};
        auto const entry = state().buffers.find(buffer_id);
        if (entry != state().buffers.end())
        {
            entry->second.released = now_seconds();
            last_sampled = entry->second.last_sampled;
            samples = entry->second.samples;
        }
    }

    trace(
        "buffer release buf={} samples={} since_last_sample={:.3f}ms",
        buffer_id, samples, (now_seconds() - last_sampled) * 1000);
}

auto md::output_key(int width, int height, int x, int y) -> std::string
{
    return std::format("{}x{}+{}+{}", width, height, x, y);
}

void md::update_output_trace(std::string const& key, std::function<void(OutputTrace&)> const& update)
{
    if (!tracing())
        return;

    install_state_dump_handler();

    std::lock_guard lock{state().mutex};
    update(state().outputs[key]);
}

void md::dump_state()
{
    std::lock_guard lock{state().mutex};

    auto const now = now_seconds();

    trace("---- state dump at {:.6f}s ----", now);
    trace(
        "concurrent-sample events: {}, max concurrent samplers of one buffer: {}",
        concurrent_sample_events(), max_concurrent_samplers());

    for (auto const& [key, output] : state().outputs)
    {
        trace(
            "output {} connector={} composites={} last_composite={:.3f}s ago "
            "last_buf={} last_tex={} elements={} bypassed={}",
            key, output.connector.empty() ? "?" : output.connector.c_str(), output.composites,
            now - output.last_composite, output.last_buffer_id, output.last_tex_id,
            output.last_element_count, output.last_bypassed);
        trace(
            "output {} posts={} skipped_posts={} failed_flips={} set_crtc_fallbacks={} "
            "scheduled_fb={} visible_fb={} last_flip={:.3f}s ago needs_set_crtc={} flips_pending={}",
            key, output.posts, output.skipped_posts, output.failed_flips, output.set_crtc_fallbacks,
            output.last_scheduled_fb, output.last_visible_fb, now - output.last_flip,
            output.needs_set_crtc, output.page_flips_pending);
        trace(
            "output {} damage_wakeups={} damage_misses={} last_damage={:.3f}s ago",
            key, output.damage_wakeups, output.damage_misses, now - output.last_damage);
    }

    for (auto const& [id, buffer] : state().buffers)
    {
        if (buffer.released != 0)
            continue;

        trace(
            "buffer {} {} modifier=0x{:016x} planes={} size={}x{} imported={:.3f}s ago "
            "samples={} last_sampled={:.3f}s ago by {}",
            id, buffer.format, buffer.modifier, buffer.planes, buffer.width, buffer.height,
            now - buffer.imported, buffer.samples,
            buffer.samples ? now - buffer.last_sampled : 0.0,
            buffer.last_sampled_by.empty() ? "-" : buffer.last_sampled_by.c_str());
    }

    trace("---- end state dump ----");
}

void md::install_state_dump_handler()
{
    static std::once_flag once;
    std::call_once(
        once,
        []
        {
            dump_fd() = ::eventfd(0, EFD_CLOEXEC);
            if (dump_fd() < 0)
            {
                mir::log(mir::logging::Severity::warning, component,
                    "Failed to create eventfd for SIGUSR2 state dumps");
                return;
            }

            std::thread{
                []
                {
                    uint64_t count{0};
                    while (::read(dump_fd(), &count, sizeof(count)) == sizeof(count))
                    {
                        dump_state();
                    }
                }}.detach();

            struct sigaction action{};
            action.sa_handler = &on_sigusr2;
            action.sa_flags = SA_RESTART;
            sigemptyset(&action.sa_mask);
            if (::sigaction(SIGUSR2, &action, nullptr) < 0)
            {
                mir::log(mir::logging::Severity::warning, component,
                    "Failed to install SIGUSR2 handler for state dumps");
                return;
            }

            mir::log(mir::logging::Severity::informational, component,
                "Send SIGUSR2 to dump per-output and per-buffer debug state");
        });
}
