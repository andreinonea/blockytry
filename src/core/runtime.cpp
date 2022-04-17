#include <core/runtime.hpp>

#include <cassert>
#include <chrono>
#include <cstddef>

namespace fost
{
namespace runtime
{

// Constant time point representing program start.
static const system_clock::time_point s_beginning = system_clock::now ();

// Constant time point representing previous program start.
static const system_clock::time_point s_prev_beginning = s_beginning; // TODO: load from configuration file.

// Time point when the latest frame was produced.
static clock::time_point s_last_frame = clock::now ();

// Time taken to produce last frame.
static std::chrono::nanoseconds s_frametime = 0ns;

// Frames per second.
static float s_fps = 0U;

void cycle ()
{
    const auto now = clock::now ();
    s_frametime = now - s_last_frame;
#if 0
    if (s_frametime != 0ns)
        s_fps = 1.0 / std::chrono::duration <float> (s_frametime).count ();
#else
    assert (s_frametime != 0ns);
    s_fps = 1.0 / std::chrono::duration <float> (s_frametime).count ();
#endif
    s_last_frame = now;
}

const system_clock::time_point beginning ()
{
    return s_beginning;
}

const system_clock::time_point previous_beginning ()
{
    return s_prev_beginning;
}

const bool is_first_run ()
{
    return (s_beginning == s_prev_beginning);
}

const system_clock::duration get ()
{
    return (system_clock::now () - s_beginning);
}

const std::chrono::nanoseconds frametime ()
{
    return s_frametime;
}

const float fps ()
{
    return s_fps;
}


} // namespace runtime
} // namespace fost
