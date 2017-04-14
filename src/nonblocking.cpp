#include <nonblocking.hpp>

namespace nonblocking
{
    search_bound::search_bound(unsigned int b, bool s) : bound{b}
    {
        if (s)
        {
            scanning = 1;
        }
        else
        {
            scanning = 0;
        }
    }

    search_bound::search_bound() : bound{0}, scanning{0} {}

    template<> versioned<unsigned int>::versioned(unsigned int version, unsigned int value) : _version{version}, _value{value} {}
    template<> versioned<unsigned int>::versioned() noexcept : _version{0}, _value{0} {}

    template<> unsigned int versioned<unsigned int>::value()
    {
        return _value;
    }

    template<> unsigned int versioned<unsigned int>::version()
    {
        return _version;
    }

    template<> bool versioned<unsigned int>::operator==(versioned<unsigned int> other)
    {
        return (_version == other._version) && (_value == other._value);
    }

    unsigned int base::hash(unsigned int key)
    {
        // Hashing the integer key gives itself -- want good scattering
        return static_cast<unsigned int>(_hash(std::to_string(key))) % size;
    }

    base::base(unsigned int size) : size{size}
    {
        bounds = static_cast<std::atomic<search_bound>* >(malloc(size * sizeof(std::atomic<search_bound>)));
        for (unsigned int i = 0; i < size; i++)
        {
            bounds[i].store(search_bound(0, false), std::memory_order_relaxed);
        }
    }

    base::~base()
    {
        free(bounds);
    }

    unsigned int base::get_probe_bound(unsigned int h)
    {
        return bounds[h].load(std::memory_order_acquire).bound;
    }

    void base::conditionally_raise_bound(unsigned int h, int index)
    {
        auto old_bound = bounds[h].load(std::memory_order_acquire);
        auto new_bound = search_bound(0, false);
        do
        {
            new_bound = search_bound(std::max(old_bound.bound, static_cast<unsigned int>(index)), false);
        }
        while (!std::atomic_compare_exchange_weak_explicit(&bounds[h], &old_bound, new_bound, std::memory_order_release, std::memory_order_acquire));
    }

    void base::conditionally_lower_bound(unsigned int h, int index)
    {
        auto old_bound = bounds[h].load(std::memory_order_acquire);
        auto new_bound = search_bound(0, false);
        if (old_bound.scanning)
        {
            new_bound = search_bound(old_bound.bound, false);
            std::atomic_compare_exchange_strong_explicit(&bounds[h], &old_bound, new_bound, std::memory_order_release, std::memory_order_relaxed);
        }
        if (index > 0)
        {
            auto bef_bound = search_bound(static_cast<unsigned int>(index), false);
            auto aft_bound = search_bound(static_cast<unsigned int>(index), true);
            auto new_bound = search_bound(0, false);
            while (std::atomic_compare_exchange_strong_explicit(&bounds[h], &bef_bound, aft_bound, std::memory_order_release, std::memory_order_relaxed))
            {
                int i = index - 1;
                while ((i > 0) && !does_bucket_contain_collision(h, i))
                {
                    i--;
                }
                new_bound = search_bound(static_cast<unsigned int>(i), false);
                std::atomic_compare_exchange_strong_explicit(&bounds[h], &aft_bound, new_bound, std::memory_order_release, std::memory_order_relaxed);
            }
        }
    }
}

