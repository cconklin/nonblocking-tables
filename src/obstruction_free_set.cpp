#include <obstruction_free_set.hpp>

namespace nonblocking
{
    obstruction_free_set::bucket::bucket(unsigned int k, bucket_state s) : key{k}
    {
        switch (s)
        {
            case empty: _state = 0; break;
            case busy: _state = 1; break;
            case inserting: _state = 2; break;
            case member: _state = 3; break;
        }
    }

    obstruction_free_set::bucket::bucket() : key{0}, _state{0} {}

    obstruction_free_set::bucket_state obstruction_free_set::bucket::state(void)
    {
        switch (_state)
        {
            case 0: return empty;
            case 1: return busy;
            case 2: return inserting;
            case 3: return member;
        }
        // Can't happen
        return busy;
    }

    std::atomic<obstruction_free_set::bucket>* obstruction_free_set::bucket_at(unsigned int h, int index)
    {
        return &buckets[(h+index*(index+1)/2)%size];
    }

    bool obstruction_free_set::does_bucket_contain_collision(unsigned int h, int index)
    {
        auto bkt = bucket_at(h, index)->load(std::memory_order_acquire);
        return (bkt.state() != empty) && (hash(bkt.key) == h);
    }

    unsigned int obstruction_free_set::hash(unsigned int key)
    {
        // Hashing the integer key gives itself -- want good scattering
        return static_cast<unsigned int>(_hash(std::to_string(key))) % size;
    }

    obstruction_free_set::obstruction_free_set(unsigned int size) : base(size)
    {
        // std::atomic and new work in clang, but not gcc...
        buckets = static_cast<std::atomic<bucket>* >(malloc(size * sizeof(std::atomic<bucket>)));
    
        for (int i = 0; i < size; ++i)
        {
            buckets[i].store(bucket(0, empty), std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_release);
    }

    obstruction_free_set::~obstruction_free_set()
    {
        free(buckets);
    }

    bool obstruction_free_set::lookup(unsigned int key)
    {
        unsigned int h = hash(key);
        unsigned int max = get_probe_bound(h);
        for (unsigned int i = 0; i <= max; ++i)
        {
            auto bkt = bucket_at(h, i)->load(std::memory_order_acquire);
            if (bkt.key == key && bkt.state() == member)
            {
                return true;
            }
        }
        return false;
    }

    bool obstruction_free_set::insert(unsigned int key)
    {
        unsigned int h = hash(key);
        unsigned int i = 0;
        auto empty_bucket = bucket(0, empty);
        auto busy_bucket = bucket(0, busy);
        while (!std::atomic_compare_exchange_strong_explicit(bucket_at(h, i), &empty_bucket, busy_bucket, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            i++;
            if (i > size)
            {
                throw "Table full";
            }
        }
        auto inserting_bkt = bucket(0, inserting);
        do
        {
            inserting_bkt = bucket(key, inserting);
            bucket_at(h, i)->store(inserting_bkt);
            conditionally_raise_bound(h, i);
            auto max = get_probe_bound(h);
            for (unsigned int j = 0; j <= max; ++j)
            {
                if (j != i)
                {
                    auto bkt_at = bucket_at(h, j)->load(std::memory_order_relaxed);
                    if (bkt_at.key == key && bkt_at.state() == inserting)
                    {
                        std::atomic_compare_exchange_strong_explicit(bucket_at(h, j), &bkt_at, bucket(0, busy), std::memory_order_release, std::memory_order_relaxed);
                    }
                    if (bkt_at.key == key && bkt_at.state() == member)
                    {
                        bucket_at(h, i)->store(bucket(0, busy), std::memory_order_release);
                        conditionally_lower_bound(h, i);
                        bucket_at(h, i)->store(bucket(0, empty), std::memory_order_relaxed);
                        return false;
                    }
                }
            }
        }
        while (!std::atomic_compare_exchange_strong_explicit(bucket_at(h, i), &inserting_bkt, bucket(key, member), std::memory_order_release, std::memory_order_relaxed));
        return true;
    }

    bool obstruction_free_set::erase(unsigned int key)
    {
        unsigned int h = hash(key);
        auto max = get_probe_bound(h);
        for (unsigned int i = 0; i <= max; ++i)
        {
            auto bkt_at = bucket_at(h, i)->load(std::memory_order_relaxed);
            if (bkt_at.key == key && bkt_at.state() == member)
            {
                if (std::atomic_compare_exchange_strong_explicit(bucket_at(h, i), &bkt_at, bucket(0, busy), std::memory_order_release, std::memory_order_relaxed))
                {
                    conditionally_lower_bound(h, i);
                    bucket_at(h, i)->store(bucket(0, empty), std::memory_order_relaxed);
                    return true;
                }
            }
        }
        return false;
    }
}

