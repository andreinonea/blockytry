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
static time_point s_last_frame = clock::now ();

// Time taken to produce last frame.
static duration s_frametime = zero;

// Frames per second.
static float s_fps = 0.0f;

void cycle ()
{
    const time_point now = clock::now ();
    s_frametime = now - s_last_frame;
#if 0
    if (s_frametime != zero)
        s_fps = 1.0f / std::chrono::duration <float> (s_frametime).count ();
#else
    assert (s_frametime != zero);
    s_fps = 1.0f / std::chrono::duration <float> (s_frametime).count ();
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

const duration frame_time ()
{
    return s_frametime;
}

const float fps ()
{
    return s_fps;
}


} // namespace runtime
} // namespace fost
