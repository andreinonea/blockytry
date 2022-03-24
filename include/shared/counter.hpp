#ifndef __BLOCKYTRY_SHARED_COUNTER_H__
#define __BLOCKYTRY_SHARED_COUNTER_H__

#include <limits>
#include <type_traits>

namespace shared
{

template <typename T>
class counter_up
{
public:
    counter_up ()
    {
        reset (static_cast <T> (0));
    }

    counter_up (T init_value, T limit)
    {
        reset (init_value, limit);
    }

    inline void reset (T init_value, T limit = std::numeric_limits <T>::max ())
    {
        static_assert (std::is_fundamental <T>::value);
        m_value = init_value;
        m_limit = limit;
    }

    inline void tick ()
    {
        if (m_value < m_limit)
            ++m_value;
        else if (m_value > m_limit)
            m_value = m_limit;

    }

    inline T tick_and_get ()
    {
        tick ();
        return m_value;
    }

    inline T get_and_tick ()
    {
        const T copy = get ();
        tick ();
        return copy;
    }

    inline T get () const { return m_value; }

private:
    T m_value;
    T m_limit;
};

template <typename T>
class counter_down
{
public:
    counter_down ()
    {
        reset (static_cast <T> (0));
    }

    counter_down (T init_value, T limit)
    {
        reset (init_value, limit);
    }

    inline void reset (T init_value, T limit = std::numeric_limits <T>::min ())
    {
        static_assert (std::is_fundamental <T>::value);
        m_value = init_value;
        m_limit = limit;
    }

    inline void tick ()
    {
        if (m_value > m_limit)
            --m_value;
        else if (m_value < m_limit)
            m_value = m_limit;

    }

    inline T tick_and_get ()
    {
        tick ();
        return m_value;
    }

    inline T get_and_tick ()
    {
        const T copy = get ();
        tick ();
        return copy;
    }

    inline T get () const { return m_value; }

private:
    T m_value;
    T m_limit;
};

} // namespace shared

#endif // __BLOCKYTRY_SHARED_COUNTER_H__