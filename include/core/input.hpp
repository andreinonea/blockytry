#ifndef _BLOCKYTRY_CORE_INPUT_H_
#define _BLOCKYTRY_CORE_INPUT_H_

#include <cassert>

#include "runtime.hpp"

namespace fost
{
namespace input
{

class key_event
{
public:
    key_event ()
        : _pressed {fost::clock::now ()}
        , _released {_pressed}
    {}

    ~key_event () = default;

    inline const fost::clock::time_point & pressed() const
    {
        return _pressed;
    }

    inline const fost::clock::time_point & released() const
    {
        return _released;
    }

    inline const bool is_complete () const
    {
        return (_released != _pressed);
    }

    template <class _Unit = std::chrono::milliseconds>
    inline const auto time_elapsed () const
    {
        return std::chrono::duration_cast <_Unit> (fost::clock::now () - _pressed);
    }

    inline const auto ticks_elapsed () const
    {
        return (time_elapsed () / fost::runtime::mspt);
    }

    template <class _Unit = std::chrono::milliseconds>
    inline const auto time_held () const
    {
        assert (is_complete ());
        return std::chrono::duration_cast <_Unit> (_released - _pressed);
    }

    inline const auto ticks_held () const
    {
        assert (is_complete ());
        return (time_held () / fost::runtime::mspt);
    }

private:
    // TODO: Friend input system class/function, not this global one.
    friend void key_callback (GLFWwindow*, int, int, int, int);

    fost::clock::time_point _pressed;
    fost::clock::time_point _released;

    inline void complete ()
    {
        assert (_released == _pressed);
        _released = fost::clock::now ();
    }
};

} // namespace input
} // namespace fost


#endif // _BLOCKYTRY_CORE_INPUT_H_