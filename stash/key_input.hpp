#include <shared/int_list.hpp>

template <int val, class Next>
using keybind = shared::int_list <val, Next>;

namespace IL = shared::IL;

/*
#define BINDINGS_ITERATOR_NEXT io::IL::null_type
#define BINDINGS_ITERATOR
#define FINAL_SHIT

template <int key>
int add_key ()
{
    if constexpr (true)
    {
        #undef BINDINGS_ITERATOR
        #define BINDINGS_ITERATOR io::IL::append <BINDINGS_ITERATOR_NEXT, key>::result

        #undef FINAL_SHIT
        #define FINAL_SHIT BINDINGS_ITERATOR

        #undef BINDINGS_ITERATOR_NEXT
        #define BINDINGS_ITERATOR_NEXT io::IL::append <BINDINGS_ITERATOR, key>::result
    }

    typedef FINAL_SHIT caca;
    std::cout << "IHAA " << io::IL::length <caca>::value << '\n';

    return key;
}


#include <vector>
#include <unordered_map>

constexpr int KEY_NOT_SET = 0xFFFF;

struct keyinfo
{
    keyinfo (int key)
        : bind(key), alt(KEY_NOT_SET), state(0) {}
  int bind;
  int alt;
  int state;
};

static std::unordered_multimap<int, keyinfo> allkeys = {};


template <int key>
int key_down ()
{
    if constexpr (true)
    {
        allkeys.emplace (std::make_pair (key, keyinfo (key)));
    }
    return key;
}

int main()
{
    key_down <1> ();
    key_down <1> ();
    key_down <1> ();
    key_down <2> ();
    key_down <3> ();
    key_down <4> ();
    
    for (const auto &v : allkeys)
        std::cout << v.first << '\n';

    return 0;
}

int main()
{
    /*
    typedef io::keybind <2, io::keybind <4, io::keybind <6, io::IL::null_type> > > pisatel;
    std::cout << io::IL::length <pisatel>::value << '\n';
    std::cout << io::IL::value_at_or_default <pisatel, 3>::value << '\n';

    typedef io::IL::append <io::IL::null_type, 8>::result cacacel;
    std::cout << io::IL::length <cacacel>::value << '\n';
    std::cout << io::IL::value_at_or_default <cacacel, 3>::value << '\n';

    typedef io::IL::append <io::IL::append <io::IL::append <io::IL::null_type, 1>::result, 2>::result, 3>::result finalkeys;
    std::cout << io::IL::value_at <finalkeys, 0>::value << '\n';
    

    // std::cout << io::add_key <10>() << '\n';
    // std::cout << io::add_key <20>() << '\n';

    // typedef FINAL_SHIT valeleu;
    // std::cout << io::IL::length <valeleu>::value << '\n';
    */

}
*/
