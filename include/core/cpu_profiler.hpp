#ifndef __STORYTAILOR_CORE_CPU_PROFILER_H__
#define __STORYTAILOR_CORE_CPU_PROFILER_H__

#include <iostream>
#include <string>


namespace fost
{

class cpu_profiler
{
    cpu_profiler () 
    {
        std::cout << "Initialized CPU profiler.\n";
    }
    
    ~cpu_profiler ()
    {
        std::cout << "Clean up CPU profiler.\n";
    }

    cpu_profiler (const cpu_profiler &other) = delete;
    cpu_profiler (cpu_profiler &&other) = delete;
    cpu_profiler & operator= (const cpu_profiler &other) = delete;
    cpu_profiler & operator= (cpu_profiler &&other) = delete;
 
public:
    static cpu_profiler & get ()
    {
        static thread_local cpu_profiler instance;
        return instance;
    }
};

const std::string & get_thread_name ();
void set_thread_name (const std::string &tname);

} // namespace fost

#endif // __STORYTAILOR_CORE_CPU_PROFILER_H__
