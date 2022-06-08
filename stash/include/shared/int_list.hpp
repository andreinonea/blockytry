#ifndef _BLOCKYTRY_IO_INT_LIST_H_
#define _BLOCKYTRY_IO_INT_LIST_H_

namespace shared
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

/*
 * Length of int_list.
 */
template <class IList> struct length;

template <>
struct length <null_type>
{
    enum { value = 0 };
};

template <int val, class Next>
struct length <int_list <val, Next>>
{
    enum { value = 1 + length <Next>::value };
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

/*
 * Get the value at index.
 */
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

/*
 * Get the value at index, returning an user-defined value if index out of bounds.
 */
template <class IList, unsigned int i, int default_val = -1>
struct value_at_or_default
{
    enum { value = default_val };
};

template <int val, class Next, int default_val>
struct value_at_or_default <int_list <val, Next>, 0, default_val>
{
    enum { value = val };
};

template <int val, class Next, unsigned int i, int default_val>
struct value_at_or_default <int_list <val, Next>, i, default_val>
{
    enum { value = value_at_or_default <Next, i - 1, default_val>::value };
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

/*
 * Get the index of target, returning an user-defined error value if target not found.
 */
template <class IList, int target, int error_val = -1> struct index_of;

template <int target, int error_val>
struct index_of <null_type, target, error_val>
{
    enum { value = error_val };
};

template <int target, class Next, int error_val>
struct index_of <int_list <target, Next>, target, error_val>
{
    enum { value = 0 };
};

template <int val, class Next, int target, int error_val>
struct index_of <int_list <val, Next>, target, error_val>
{
private:
    enum { temp = index_of <Next, target, error_val>::value };
public:
    enum { value = (temp == error_val) ? error_val : 1 + temp };
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

/*
 * Append another int_list by creating a new int_list.
 */
template <class IList, int appended> struct append;

template <int appended>
struct append <null_type, appended>
{
    typedef int_list <appended, null_type> result;
};

template <int val, class Next, int appended>
struct append <int_list <val, Next>, appended>
{
    typedef int_list <val, typename append <Next, appended>::result > result;
};

} // namespace IL (IntList)

} // namespace shared

#endif // _BLOCKYTRY_IO_INT_LIST_H_
