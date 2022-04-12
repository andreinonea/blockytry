#ifndef __BLOCKYTRY_IO_LOOKUP_TABLE_H__
#define __BLOCKYTRY_IO_LOOKUP_TABLE_H__

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace shared
{

template <typename _Key, typename _Value, std::size_t size>
using lut = std::array <std::pair <_Key, _Value>, size>;

template <typename _Key, typename _Value, std::size_t size>
struct __lookup_table_impl
{
    lut <_Key, _Value, size> data;

    [[nodiscard]] consteval _Value at (const _Key &key) const
    {
        const auto itr = std::find_if (std::begin (data), std::end (data),
            [&key] (const auto &v) { return (v.first == key); });
        if (itr == std::end (data))
            throw std::range_error ("Key not found in lookup_table.");
        
        return itr->second;
    }
};

template <typename _Key, typename _Value, std::size_t size>
consteval _Value lookup (const lut <_Key, _Value, size> &lut, const _Key &key)
{
    return __lookup_table_impl <_Key, _Value, size> {{lut}}.at (key);
}

template <typename _Value, std::size_t size>
consteval _Value lookup (const lut <std::string_view, _Value, size> &lut, const char *key)
{
    return __lookup_table_impl <std::string_view, _Value, size> {{lut}}.at (key);
}

} // namespace shared

#endif // __BLOCKYTRY_IO_LOOKUP_TABLE_H__