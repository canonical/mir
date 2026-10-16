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

#ifndef MIR_DEBUG_FRAME_TRACE_H_
#define MIR_DEBUG_FRAME_TRACE_H_

#include <atomic>
#include <cstdint>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

/* THROWAWAY DEBUG INSTRUMENTATION for https://github.com/canonical/mir/issues/5325
 *
 * Everything here is controlled by two environment variables, read once:
 *
 *   MIR_DEBUG_BUFFER_SYNC = off | per-buffer | per-buffer-finish | global | global-finish | reimport
 *   MIR_DEBUG_FRAME_TRACE = 0 | 1 | 2
 *
 * Send SIGUSR2 to the server to dump per-output and per-buffer state.
 */
namespace mir
{
namespace debug
{
enum class BufferSyncMode
{
    off,                ///< Unmodified Mir behaviour
    per_buffer,         ///< Mutual exclusion while sampling a given client buffer
    per_buffer_finish,  ///< …and glFinish() before releasing, so the GPU read has completed
    global,             ///< One process-wide mutex around all compositing and screen capture
    global_finish,      ///< …and glFinish() before releasing
    reimport            ///< Fresh EGLImage + texture for every composite
};

auto buffer_sync_mode() -> BufferSyncMode;

/// 0 = no tracing, 1 = per-frame tracing, 2 = + per-sample tracing
auto frame_trace_level() -> int;

inline auto tracing() -> bool
{
    return frame_trace_level() > 0;
}

inline auto tracing_verbose() -> bool
{
    return frame_trace_level() > 1;
}

/// Seconds since the first call into this module; the common clock for all trace lines
auto now_seconds() -> double;

void trace_message(std::string_view message);
void trace_warning_message(std::string_view message);

template<typename... Args>
void trace(std::format_string<Args...> fmt, Args&&... args)
{
    trace_message(std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void trace_warning(std::format_string<Args...> fmt, Args&&... args)
{
    trace_warning_message(std::format(fmt, std::forward<Args>(args)...));
}

/* Tier B: concurrency detector.
 *
 * `counter` lives in the shared texture object; these calls never change behaviour,
 * they only report when more than one thread is sampling the same buffer at once.
 */
void note_sample_begin(std::atomic<int>& counter, uint32_t buffer_id, unsigned tex_id);
void note_sample_end(std::atomic<int>& counter, uint32_t buffer_id);

auto concurrent_sample_events() -> uint64_t;
auto max_concurrent_samplers() -> int;

/// The mutex used by the `global` and `global-finish` modes
auto global_render_mutex() -> std::mutex&;

/// glFinish(), for callers that do not otherwise pull in GL headers
void gl_finish();

/* Tier D: client buffer lifecycle */
void note_buffer_imported(
    uint32_t buffer_id,
    std::string_view format,
    uint64_t modifier,
    unsigned planes,
    int width,
    int height);
void note_buffer_sampled(uint32_t buffer_id, std::string_view output);
void note_buffer_released(uint32_t buffer_id);

/* Tier G: state dumped on SIGUSR2 */
struct OutputTrace
{
    std::string connector;
    uint64_t composites{0};
    double last_composite{0};
    uint32_t last_buffer_id{0};
    unsigned last_tex_id{0};
    unsigned last_element_count{0};
    bool last_bypassed{false};
    uint64_t posts{0};
    uint64_t skipped_posts{0};
    uint64_t failed_flips{0};
    uint64_t set_crtc_fallbacks{0};
    uint32_t last_scheduled_fb{0};
    uint32_t last_visible_fb{0};
    double last_flip{0};
    bool needs_set_crtc{false};
    bool page_flips_pending{false};
    double last_damage{0};
    uint64_t damage_wakeups{0};
    uint64_t damage_misses{0};
};

/// `key` is the output's view area, formatted by `output_key()`, so that the
/// compositor side and the KMS side agree on a name for the same output.
void update_output_trace(std::string const& key, std::function<void(OutputTrace&)> const& update);

auto output_key(int width, int height, int x, int y) -> std::string;

void dump_state();

/// Idempotent; installs the SIGUSR2 handler on first call
void install_state_dump_handler();
}
}

#endif // MIR_DEBUG_FRAME_TRACE_H_
