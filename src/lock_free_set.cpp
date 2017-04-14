#include <lock_free_set.hpp>

namespace nonblocking
{
    template<> versioned<lock_free_set::bucket_state>::versioned(unsigned int version, lock_free_set::bucket_state value) : _version{version}
    {
        switch (value)
        {
            case lock_free_set::bucket_state::empty: _value = 0; break;
            case lock_free_set::bucket_state::busy: _value = 1; break;
            case lock_free_set::bucket_state::collided: _value = 2; break;
            case lock_free_set::bucket_state::visible: _value = 3; break;
            case lock_free_set::bucket_state::inserting: _value = 4; break;
            case lock_free_set::bucket_state::member: _value = 5; break;
        }
    }
    template<> versioned<lock_free_set::bucket_state>::versioned() noexcept : _version{0}, _value{0} {}
    
    template<> bool versioned<lock_free_set::bucket_state>::operator==(versioned<lock_free_set::bucket_state> other)
    {
        return (_version == other._version) && (_value == other._value);
    }
    
    template<> lock_free_set::bucket_state versioned<lock_free_set::bucket_state>::value()
    {
        switch (_value)
        {
            case 0: return lock_free_set::bucket_state::empty;
            case 1: return lock_free_set::bucket_state::busy;
            case 2: return lock_free_set::bucket_state::collided;
            case 3: return lock_free_set::bucket_state::visible;
            case 4: return lock_free_set::bucket_state::inserting;
            case 5: return lock_free_set::bucket_state::member;
            default: return lock_free_set::bucket_state::busy;
        }
    }

    template<> unsigned int versioned<lock_free_set::bucket_state>::version()
    {
        return _version;
    }

    lock_free_set::bucket::bucket(unsigned int key, unsigned int version, lock_free_set::bucket_state state) : key{key}, vs(versioned<lock_free_set::bucket_state>(version, state)) {}

    lock_free_set::bucket::bucket() : key{0}, vs(versioned<lock_free_set::bucket_state>()) {}

    lock_free_set::bucket* lock_free_set::bucket_at(unsigned int h, int index)
    {
        return &buckets[(h+index*(index+1)/2)%size];
    }

    bool lock_free_set::does_bucket_contain_collision(unsigned int h, int index)
    {
        auto state = bucket_at(h, index)->vs.load(std::memory_order_acquire);
        if ((state.value() == visible) || (state.value() == inserting) || (state.value() == member))
        {
            if (hash(bucket_at(h, index)->key.load(std::memory_order_acquire)) == h)
            {
                auto state2 = bucket_at(h, index)->vs.load(std::memory_order_relaxed);
                if ((state2.value() == visible) || (state2.value() == inserting) || (state2.value() == member))
                {
                    if (state2.version() == state.version())
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    lock_free_set::lock_free_set(unsigned int size) : base(size)
    {
        buckets = new bucket[size];
        std::atomic_thread_fence(std::memory_order_release);
    }

    bool lock_free_set::lookup(unsigned int key)
    {
        unsigned int h = hash(key);
        auto max = get_probe_bound(h);
        for (unsigned int i = 0; i <= max; ++i)
        {
            auto state = bucket_at(h, i)->vs.load(std::memory_order_acquire);
            if ((state.value() == member) && (bucket_at(h, i)->key.load(std::memory_order_acquire) == key))
            {
                auto state2 = bucket_at(h, i)->vs.load(std::memory_order_relaxed);
                if (state == state2)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool lock_free_set::erase(unsigned int key)
    {
        unsigned int h = hash(key);
        auto max = get_probe_bound(h);
        for (unsigned int i = 0; i <= max; ++i)
        {
            auto* bucket = bucket_at(h, i);
            auto state = bucket->vs.load(std::memory_order_acquire);
            if ((state.value() == member) && (bucket->key.load(std::memory_order_acquire) == key))
            {
                if (std::atomic_compare_exchange_strong_explicit(&bucket->vs, &state, versioned<bucket_state>(state.version(), busy), std::memory_order_release, std::memory_order_relaxed))
                {
                    conditionally_lower_bound(h, i);
                    bucket->vs.store(versioned<bucket_state>(state.version() + 1, empty), std::memory_order_release);
                    return true;
                }
            }
        }
        return false;
    }

    bool lock_free_set::insert(unsigned int key)
    {
        unsigned int h = hash(key);
        int i = -1;
        unsigned int version;
        versioned<bucket_state> old_vs;
        versioned<bucket_state> new_vs;
        do
        {
            if (++i >= size)
            {
                throw "Table full";
            }
            version = bucket_at(h, i)->vs.load(std::memory_order_acquire).version();
            old_vs = versioned<bucket_state>(version, empty);
            new_vs = versioned<bucket_state>(version, busy);
        }
        while (!std::atomic_compare_exchange_strong_explicit(&bucket_at(h, i)->vs, &old_vs, new_vs, std::memory_order_release, std::memory_order_relaxed));
        bucket_at(h, i)->key.store(key, std::memory_order_relaxed);
        while (true)
        {
            bucket_at(h, i)->vs.store(versioned<bucket_state>(version, visible), std::memory_order_release);
            conditionally_raise_bound(h, i);
            bucket_at(h, i)->vs.store(versioned<bucket_state>(version, inserting), std::memory_order_release);
            auto r = assist(key, h, i, version);
            if (!(bucket_at(h, i)->vs.load(std::memory_order_acquire) == versioned<bucket_state>(version, collided)))
            {
                return true;
            }
            if (!r)
            {
                conditionally_lower_bound(h, i);
                bucket_at(h, i)->vs.store(versioned<bucket_state>(version+1, empty), std::memory_order_release);
                return false;
            }
            version++;
        }
    }

    bool lock_free_set::assist(unsigned int key, unsigned int h, int i, unsigned int ver_i)
    {
        auto max = get_probe_bound(h);
        versioned<bucket_state> old_vs(ver_i, inserting);
        for (unsigned int j = 0; j <= max; j++)
        {
            if (i != j)
            {
                auto* bkt_at = bucket_at(h, j);
                auto state = bkt_at->vs.load(std::memory_order_acquire);
                if ((state.value() == inserting) && (bkt_at->key.load(std::memory_order_acquire) == key))
                {
                    if (j < i)
                    {
                        auto j_state = bkt_at->vs.load(std::memory_order_relaxed);
                        if (j_state == state)
                        {
                            std::atomic_compare_exchange_strong_explicit(&bucket_at(h, i)->vs, &old_vs, versioned<bucket_state>(ver_i, collided), std::memory_order_relaxed, std::memory_order_relaxed);
                            return assist(key, h, j, state.version());
                        }
                    }
                    else
                    {
                        auto i_state = bucket_at(h, i)->vs.load(std::memory_order_acquire);
                        if (i_state == versioned<bucket_state>(ver_i, inserting))
                        {
                            std::atomic_compare_exchange_strong_explicit(&bkt_at->vs, &state, versioned<bucket_state>(state.version(), collided), std::memory_order_relaxed, std::memory_order_relaxed);
                        }
                    }
                    auto new_state = bkt_at->vs.load(std::memory_order_acquire);
                    if ((new_state.value() == member) && (bkt_at->key.load(std::memory_order_acquire) == key))
                    {
                        if (bkt_at->vs.load(std::memory_order_relaxed) == new_state)
                        {
                            std::atomic_compare_exchange_strong_explicit(&bucket_at(h, i)->vs, &old_vs, versioned<bucket_state>(ver_i, collided), std::memory_order_relaxed, std::memory_order_relaxed);
                            return false;
                        }
                    }
                }
            }
        }
        std::atomic_compare_exchange_strong_explicit(&bucket_at(h, i)->vs, &old_vs, versioned<bucket_state>(ver_i, member), std::memory_order_release, std::memory_order_relaxed);
        return true;
    }

    lock_free_set::~lock_free_set()
    {
        delete buckets;
    }
}

