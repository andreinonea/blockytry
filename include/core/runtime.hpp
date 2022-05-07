#ifndef __STORYTAILOR_CORE_RUNTIME_H__
#define __STORYTAILOR_CORE_RUNTIME_H__

#include <chrono>
#include <cstddef>

using namespace std::chrono_literals;

namespace fost
{
// TODO: write real documentation after this is done....!

// Counts real world time. Should be used when synchronization with the
// real world is expected and desired. It is similar to how a watch tells time.
// https://stackoverflow.com/questions/31552193/difference-between-steady-clock-vs-system-clock#:~:text=Answering%20questions%20in,current%20time%20is.
using system_clock = std::chrono::system_clock;

// Counts program time. Use this for all timing related program logic. It is
// similar to how a stopwatch measures time points. Safe to use in cycle or tick methods.
// https://stackoverflow.com/questions/31552193/difference-between-steady-clock-vs-system-clock#:~:text=Answering%20questions%20in,current%20time%20is.
using clock = std::chrono::steady_clock;

namespace runtime
{

// Ticks per second - how many ticks happen in 1 second.
// 50 milliseconds per tick. 1/50 = 0.02 ticks per millisecond = 20 tps.
constexpr std::uint16_t tps = 20U;

// Milliseconds per tick - interval at which most game systems are ticked.
// 20 ticks per second. 1/20 = 0.05 seconds per tick = 50 mspt.
constexpr std::chrono::milliseconds mspt = 1000ms / tps;

using ticks = std::chrono::duration <std::int64_t, std::ratio <1, tps>>;
constexpr ticks tick_unit = ticks{1};

using duration = decltype (clock::duration {} + tick_unit);
using time_point = std::chrono::time_point <clock, duration>;
constexpr duration zero = duration{0};

// Called each frame to update its delta time.
void cycle ();

// Time point at which the program started.
const system_clock::time_point beginning ();

// Time point at which the program previously started.
const system_clock::time_point previous_beginning ();

// Returns true if this is the first time the program is run.
const bool is_first_run ();

// Time since program started.
const system_clock::duration get ();

// Time taken to produce last frame (delta time).
const duration frame_time ();

// Frames per second.
const float fps ();

} // namespace runtime
} // namespace fost

#endif // __STORYTAILOR_CORE_RUNTIME_H__