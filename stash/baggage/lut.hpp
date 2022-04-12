/*
static auto allkeys = std::vector <int> {};

int key_down (int key)
{
    if constexpr (true)
    {
        if (std::find (std::begin (allkeys), std::end (allkeys), key) == std::end (allkeys))
        {
            allkeys.push_back (key);
        }
    }
    return key;
}

using namespace std::string_view_literals;

static constexpr auto actions = shared::lut <std::string_view, int, 3>
    {{
        { "jump"sv, GLFW_KEY_SPACE },
        { "walk"sv, GLFW_KEY_W },
        { "run"sv, GLFW_KEY_LEFT_SHIFT }
    }};

static constexpr shared::lut <int, int, 3> actions2 = 
    {{
        { GLFW_KEY_SPACE, 2 },
        { GLFW_KEY_W, 3 },
        { GLFW_KEY_LEFT_SHIFT, 1 }
    }};

static constexpr auto actions3 = shared::lut <int, std::pair <int, int>, 3>
    {{
        { 0, std::make_pair (10, 20) },
        { 3, std::make_pair (40, 50) },
        { 6, std::make_pair (70, 80) }
    }};

const int buckets (const int keys_count)
{
    auto buckets = keys_count / 8;
    return (keys_count % 8) ? buckets + 1 : buckets;
}

int main()
{
    std::cout << "CRAPPY BUILD!!!\n";

    auto key = int {shared::lookup (actions2, shared::lookup (actions, "jump"))};
    std::cout << key << '\n';

    key = shared::lookup (actions2, shared::lookup (actions, "walk"));
    std::cout << key << '\n';

    key = shared::lookup (actions2, shared::lookup (actions, "run"));
    std::cout << key << '\n';

    std::cout << shared::lookup (actions3, 0).first << '\n';

    std::cout << key_down (shared::lookup (actions, "jump")) << '\n';
    key_down (2);
    key_down (10);
    key_down (2412);
    key_down (2412);
    key_down (2412);

    std::cout << "----\n";
    for (const auto v : allkeys)
    {
        std::cout << v << " ";
    }
    std::cout << "\n----\n";

    std::cout << buckets (199) << '\n';
    std::cout << buckets (200) << '\n';
    std::cout << buckets (201) << '\n';

    const auto num_buckets = buckets (allkeys.size ());
    auto appkeys = std::array <std::uint8_t, num_buckets> {};
    std::cout << num_buckets << '\n';

    return 0;
*/
