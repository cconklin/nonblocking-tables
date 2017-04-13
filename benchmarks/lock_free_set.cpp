#include <iostream>
#include <lock_free_set.hpp>

int main(int argc, const char *argv[])
{
    nonblocking::lock_free_set lockset(32);
    std::cout << lockset.lookup(5) << std::endl;
    std::cout << lockset.insert(5) << std::endl;
    std::cout << lockset.lookup(5) << std::endl;
    std::cout << lockset.erase(5) << std::endl;
    std::cout << lockset.lookup(5) << std::endl;
    return 0;
}

