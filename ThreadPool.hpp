#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_queue.h>

#include "Worker.hpp"

class ThreadPool {
public:
    // ----- Construction API -----

    // Construct with the start and end range that should be checked for primeness
    // This will print every prime number in the range [start, end)
    // Will use numThreads threads to solve
    ThreadPool(uint64_t start, uint64_t end, std::size_t numThreads);
    ~ThreadPool();

    // Call to begin processing
    void start();

    // ----- End Construction API -----
    // ----- Thread API -----

    // Try to work from the work queue
    // If this returns false but m_addingWork is still true then try again
    bool getWork(uint64_t& out);

    // Add a number to the results table
    void setResult(uint64_t vaue);

    // Check if a number is prime
    // This is only correct for numbers between m_checkpoint[1] and m_checkpoint[2]
    void isPrime(uint64_t value);

    // Will be true as long as more work is being added during this checkpoint
    std::atomic_bool m_addingWork;

    // Threads should use this CV to wait when they are done work and work is not being added
    std::condition_variable m_threadCV;

    // Acquire this mutex before doing anything with m_poolCV
    std::mutex m_mutex;

    // Threads should signal this CV before waiting on m_threadCV
    std::condition_variable m_poolCV;

    // ----- End Thread API -----
private:
    // Add work to the work queue
    void addWork(uint64_t next);

    /*
     * Performs a rotation of the 3 results tables.
     * m_resultsTable is set to newResults.
     * m_nonPrime is set to m_resultsTable.
     * m_nonPrime is returned
     * This should only be called when all threads have finished their work and are waiting on m_threadCV
     */
    std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> swapResults(std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> newResults);

    // Update m_checkpoints and all thread checkpoints at the same time
    void updateCheckpoints();

    // The current queue of work to be done
    tbb::concurrent_queue<uint64_t> m_workQueue;

    std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> m_resultsTable;
    std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> m_nonPrime;

    // The first number to check for primeness
    uint64_t m_startRange;
    // One after the last number to check for primeness
    uint64_t m_endRange;

    // The current checkpoints
    // [checkpoint[0], checkpoint[1]) is the range that threads are processing
    // [checkpoint[1], checkpoint[2]) is the range that can be checked for primeness
    // [checkpoint[2], checkpoint[3]) is the range that will be printed to stdout this iteration
    uint64_t m_checkpoint[4];

    // The running threads
    std::vector<Worker> m_workers;
};

#endif // __THREAD_POOL_HPP__