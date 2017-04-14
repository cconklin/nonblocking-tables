#pragma once
#include <nonblocking.hpp>

namespace nonblocking
{
    class obstruction_free_dictionary : public base
    {
    public:

        enum bucket_state
        {
            empty,
            busy,
            inserting,
            member,
            updating
        };

        struct bucket
        {
            std::atomic<unsigned int> key;
            std::atomic<versioned<bucket_state> > state;
            std::atomic<versioned<unsigned int> > value;
            std::atomic<versioned<unsigned int> > copy;
            bucket(unsigned int key, unsigned int value, unsigned int version, bucket_state state);
            bucket(unsigned int key, versioned<unsigned int> value, versioned<unsigned int> copy, versioned<bucket_state> state);
            bucket();
            bucket(const bucket& b);
        };

    protected:
    
        bucket* buckets;

        bucket* bucket_at(unsigned int h, int index);
        bool does_bucket_contain_collision(unsigned int h, int index);
        std::pair<bucket*, bucket> read_bucket(unsigned int key);
    
    public:

        obstruction_free_dictionary(unsigned int size);
        ~obstruction_free_dictionary();
        std::pair<bool, unsigned int> lookup(unsigned int key);
        void set(unsigned int key, unsigned int value);
        bool erase(unsigned int key);
    
    };
}

