#include <iostream>
#include <obstruction_free_set.hpp>

int main(int argc, const char *argv[])
{
    nonblocking::obstruction_free_set obset(32);
    std::cout << obset.lookup(5) << std::endl;
    std::cout << obset.insert(5) << std::endl;
    std::cout << obset.lookup(5) << std::endl;
    std::cout << obset.erase(5) << std::endl;
    std::cout << obset.lookup(5) << std::endl;
    return 0;
}

