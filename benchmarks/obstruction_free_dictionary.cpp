#include <iostream>
#include <obstruction_free_dictionary.hpp>

int main(int argc, const char *argv[])
{
    nonblocking::obstruction_free_dictionary obmap(32);
    auto look = obmap.lookup(5);
    if (std::get<0>(look))
    {
        std::cout << "(fail) empty map contains 5 (value = " << std::get<1>(look) << ")" << std::endl;
    }
    else
    {
        std::cout << "empty map does not contain 5" << std::endl;
    }
    obmap.set(5, 11);
    look = obmap.lookup(5);
    if (std::get<0>(look))
    {
        std::cout << "map contains 5 (value = " << std::get<1>(look) << ")" << std::endl;
    }
    else
    {
        std::cout << "(fail) map does not contain 5" << std::endl;
    }
    obmap.set(5, 18);
    look = obmap.lookup(5);
    if (std::get<0>(look))
    {
        std::cout << "map contains 5 (value = " << std::get<1>(look) << ")" << std::endl;
    }
    else
    {
        std::cout << "(fail) map does not contain 5" << std::endl;
    }
    std::cout << obmap.erase(5) << std::endl;
    look = obmap.lookup(5);
    if (std::get<0>(look))
    {
        std::cout << "(fail) empty map contains 5 (value = " << std::get<1>(look) << ")" << std::endl;
    }
    else
    {
        std::cout << "empty map does not contain 5" << std::endl;
    }
    return 0;
}

