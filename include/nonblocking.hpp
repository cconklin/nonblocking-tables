#pragma once
#include <atomic>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <cstdlib>

namespace nonblocking
{
    struct search_bound
    {
        unsigned int bound : 31, scanning : 1;
        search_bound(unsigned int b, bool s);
        search_bound();
    };

    class base
    {
        std::atomic<search_bound>* bounds; // <bound, scanning>
    protected:
        unsigned int size;
        unsigned int get_probe_bound(unsigned int h);
        void conditionally_raise_bound(unsigned int h, int index);
        void conditionally_lower_bound(unsigned int h, int index);
        virtual bool does_bucket_contain_collision(unsigned int h, int index) = 0;
    public:
        base(unsigned int size);
        ~base();
    };
}    

