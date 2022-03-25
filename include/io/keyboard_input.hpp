#ifndef __BLOCKYTRY_IO_KEYBOARD_INPUT_H__
#define __BLOCKYTRY_IO_KEYBOARD_INPUT_H__

namespace io
{

template <int val, class Next>
struct int_list
{
    typedef Next next;
};

namespace IL
{

class null_type {};
struct empty_type {};

// Length of int_list.
template <class IList> struct length;
template <> struct length <null_type>
{
    enum { value = 0 };
};
template <int val, class Next>
struct length <int_list <val, Next>>
{
    enum { value = 1 + length <Next>::value };
};

// Get the value at index
template <class IList, unsigned int i> struct value_at;
template <int val, class Next>
struct value_at <int_list <val, Next>, 0>
{
    enum { value = val };
};
template <int val, class Next, unsigned int i>
struct value_at <int_list <val, Next>, i>
{
    enum { value = value_at <Next, i - 1>::value };
};

} // namespace IL (IntList)

} // namespace io

#endif // __BLOCKYTRY_IO_KEYBOARD_INPUT_H__
