#include <core/cpu_profiler.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <thread>

namespace fost
{

namespace // anonymous
{
static const std::string tid_to_string (const std::thread::id &tid)
{
    std::stringstream ss;
    ss << tid;
    return ss.str ();
}
} // namespace anonymous

static thread_local std::string tl_thread_name = tid_to_string (std::this_thread::get_id ());

const std::string & get_thread_name ()
{
    return tl_thread_name;
}

void set_thread_name (const std::string &tname)
{
    tl_thread_name = tname;
}

} // namespace fost
