#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <obstruction_free_dictionary.hpp>

void bm_func(nonblocking::obstruction_free_dictionary& dict, int ops, int reads, int writes, int deletes, std::atomic<bool>& start)
{
    while (!start.load());
    for (unsigned int i = 0; i < ops; ++i)
    {
        for (unsigned int j = 0; j < writes; ++j)
        {
            dict.set(static_cast<unsigned int>(rand()) % 2048, j);
        }
        for (unsigned int j = 0; j < reads; ++j)
        {
            dict.lookup(static_cast<unsigned int>(rand()) % 2048);
        }
        for (unsigned int j = 0; j < deletes; ++j)
        {
            dict.erase(static_cast<unsigned int>(rand()) % 2048);
        }
    }
}

double benchmark(int t, int ops, int reads, int writes, int deletes)
{
    std::atomic<bool> start;
    std::vector<std::thread> threads;
    nonblocking::obstruction_free_dictionary obmap(32768);

    for (int i = 0; i < t; ++i)
    {
        threads.push_back(std::thread(bm_func, std::ref(obmap), ops, reads, writes, deletes, std::ref(start)));
    }

    auto run_start = std::chrono::high_resolution_clock::now();

    start.store(true);

    for (auto& thread : threads)
    {
        thread.join();
    }

    auto run_stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> run_time = run_stop - run_start;

    return run_time.count();
}

int main(int argc, const char *argv[])
{
    auto reads = 8;
    auto writes = 1;
    auto deletes = 1;
    std::vector<int> threads = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128};
    for (auto t : threads)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto ops = 640000 / t;
            auto runtime = benchmark(t, ops, reads, writes, deletes);
            std::cout << t << " Microseconds/Op: " << 1000000.0 * runtime / static_cast<double>(ops*t) << std::endl;
        }
    }
    return 0;
}

