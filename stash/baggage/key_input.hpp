/*#include <shared/int_list.hpp>

template <int val, class Next>
using keybind = shared::int_list <val, Next>;

namespace IL = shared::IL;

int main()
{
    
    typedef io::keybind <2, io::keybind <4, io::keybind <6, io::IL::null_type> > > pisatel;
    std::cout << io::IL::length <pisatel>::value << '\n';
    std::cout << io::IL::value_at_or_default <pisatel, 3>::value << '\n';

    typedef io::IL::append <io::IL::null_type, 8>::result cacacel;
    std::cout << io::IL::length <cacacel>::value << '\n';
    std::cout << io::IL::value_at_or_default <cacacel, 3>::value << '\n';

    typedef io::IL::append <io::IL::append <io::IL::append <io::IL::null_type, 1>::result, 2>::result, 3>::result finalkeys;
    std::cout << io::IL::value_at <finalkeys, 0>::value << '\n';
}
*/
