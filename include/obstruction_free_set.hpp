#pragma once
#include <nonblocking.hpp>

namespace nonblocking
{
    class obstruction_free_set : public base
    {
        enum bucket_state
        {
            empty,
            busy,
            inserting,
            member
        };

        struct bucket
        {
            unsigned int key : 30, _state : 2;
            bucket(unsigned int k, bucket_state s);
            bucket();
            bucket_state state(void);
        };

        std::atomic<bucket>* buckets; // <key, state>
    
        std::atomic<bucket>* bucket_at(unsigned int h, int index);
        bool does_bucket_contain_collision(unsigned int h, int index);
    
    public:
    
        obstruction_free_set(unsigned int size);
        ~obstruction_free_set();
        bool lookup(unsigned int key);
        bool insert(unsigned int key);
        bool erase(unsigned int key);
    
    };
}

