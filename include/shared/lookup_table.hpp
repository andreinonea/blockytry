#ifndef __BLOCKYTRY_IO_LOOKUP_TABLE_H__
#define __BLOCKYTRY_IO_LOOKUP_TABLE_H__

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace shared
{

template <typename Key, typename Value, std::size_t Size>
using map = std::array <std::pair <Key, Value>, Size>;

template <typename Key, typename Value, std::size_t Size>
struct lookup_table
{
    shared::map <Key, Value, Size> data;

    [[nodiscard]] consteval Value at (const Key &key) const
    {
        const auto itr = std::find_if (begin (data), end (data),
            [&key] (const auto &v) { return (v.first == key); });
        if (itr == end (data))
            throw std::range_error ("Value not found in lookup_table.");
        
        return itr->second;
    }
};

template <typename _Key, typename _Value, std::size_t size>
consteval _Value lookup (const shared::map <_Key, _Value, size> &map, _Key key)
{
    return shared::lookup_table <_Key, _Value, size> {{map}}.at (key);
}

template <typename _Value, std::size_t size>
consteval _Value lookup (const shared::map <std::string_view, _Value, size> &map, const char *key)
{
    return shared::lookup_table <std::string_view, _Value, size> {{map}}.at (key);
}

} // namespace shared

#endif // __BLOCKYTRY_IO_LOOKUP_TABLE_H__