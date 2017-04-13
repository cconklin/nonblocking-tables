#pragma once
#include <nonblocking.hpp>

namespace nonblocking
{
    class lock_free_set : public base
    {
    public:

        enum bucket_state
        {
            empty,
            busy,
            collided,
            visible,
            inserting,
            member
        };

    protected:
        
        struct bucket
        {
            std::atomic<versioned<bucket_state> > vs;
            std::atomic<unsigned int> key;
            bucket(unsigned int key, unsigned int version, bucket_state state);
            bucket();
        };

        bucket* buckets;

        bucket* bucket_at(unsigned int h, int index);
        bool does_bucket_contain_collision(unsigned int h, int index);
        bool assist(unsigned int key, unsigned int h, int i, unsigned int ver_i);
    public:

        lock_free_set(unsigned int size);
        ~lock_free_set();
        bool lookup(unsigned int key);
        bool insert(unsigned int key);
        bool erase(unsigned int key);

    };
}

