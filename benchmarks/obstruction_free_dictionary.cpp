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

void calibrate(int ops, std::atomic<bool>& start)
{
    while (!start.load());
    volatile unsigned int x;
    for (unsigned int i = 0; i < ops; ++i)
    {
        x = static_cast<unsigned int>(rand()) % 2048;
    }
}

double benchmark(int t, int ops, int reads, int writes, int deletes)
{
    std::atomic<bool> start;
    std::vector<std::thread> threads;
    std::vector<std::thread> cal_threads;
    nonblocking::obstruction_free_dictionary obmap(32768);

    // Calibration run

    start.store(false);

    for (int i = 0; i < t; ++i)
    {
        cal_threads.push_back(std::thread(calibrate, ops*(reads + writes + deletes), std::ref(start)));
    }
    auto cal_start = std::chrono::high_resolution_clock::now();

    start.store(true);

    for (auto& thread : cal_threads)
    {
        thread.join();
    }
    auto cal_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> cal_time = cal_stop - cal_start;

    start.store(false);

    // Actual Run
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

    return run_time.count() - cal_time.count();
}

int main(int argc, const char *argv[])
{
    auto ops = 1000000;
    auto reads = 8;
    auto writes = 1;
    auto deletes = 1;
    std::vector<int> threads = {1, 2, 3, 4, 6, 8, 12, 16, 32, 64, 96, 128};
    for (auto t : threads)
    {
        auto runtime = benchmark(t, ops, reads, writes, deletes);
        std::cout << "[" << t << "] Microseconds / Op: " << 1000000.0 * runtime / static_cast<double>(ops) << std::endl;
    }
    return 0;
}

