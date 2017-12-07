#include "ThreadPool.hpp"

static const uint64_t CHECKPOINT_SIZE = 20; // TODO This number is good for testing but should be larger or configurable

ThreadPool::ThreadPool(uint64_t start, uint64_t end, std::size_t numThreads) :
    m_addingWork(false),
    m_threadsShouldExit(false),
    m_resultsTable(nullptr),
    m_nonPrime(nullptr),
    m_startRange(start),
    m_endRange(end),
    m_checkpoint{start, start, start, start} {
    // Create Workers
    for (int i = 0; i < numThreads; i++) {
        // TODO add worker ctor parameters after writing that api
        m_workers.emplace_back();
    }
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::start() {
    // Update tables to start state
    m_resultsTable = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();
    m_nonPrime = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();

    auto toBePrinted = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();

    // Update checkpoints for start
    updateCheckpoint();

    // Start the workers running
    for (Worker& w : m_workers) {
        w.start();
    }

    while (!doneProcessing()) {
        // Signal workers to wake up and begin processing
        m_workerCV.notify_all();

        uint64_t checkpoint_try = m_checkpoint[0] / 3 + 1;
        for (
            // Start at first odd number after end of last checkpoint
            uint64_t i = m_checkpoint[1] + ((m_checkpoint[1] + 1) % 2);
            i < checkpoint_try; // Iterate over every number less than sqrt(checkpoint)
            i = i + 2 // Iterate over odd numbers
            ) {
            addWork(i);
        }
        std::atomic_thread_fence(std::memory_order_release);
        m_addingWork.store(false, std::memory_order_relaxed);

        // drain toBePrinted

        // wait for Workers
        waitForWorkers();

        // update checkpoint
        updateCheckpoint();

        auto newTable = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();
        toBePrinted = swapResults(std::move(newTable));

        // unset done
        std::atomic_thread_fence(std::memory_order_release);
        m_addingWork.store(false, std::memory_order_relaxed);
    }
    // set done all
    std::atomic_thread_fence(std::memory_order_release);
    m_threadsShouldExit.store(true, std::memory_order_relaxed);

    // signal Workers to wake up and begin processing. They will see that they should exit and join up.
    m_workerCV.notify_all();

    // drain toBePrinted
    // drain m_nonPrime
    // drain m_resultsTable

    // cleanup
    for (Worker& w : m_workers) {
        w.finish();
    }
}

bool ThreadPool::getWork(uint64_t& out) {
    return m_workQueue.try_pop(out);
}

void ThreadPool::setResult(uint64_t value) {
    m_resultsTable.insert(value);
}

bool ThreadPool::isPrime(uint64_t value) {
    return m_nonPrime.count(value);
}

void ThreadPool::addWork(uint64_t next) {
    m_workQueue.push(next);
}

std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> ThreadPool::swapResults(std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>> newResults) {
    m_nonPrime.swap(m_resultsTable);
    m_resultsTable.swap(newResults);
    return newResults;
}

void ThreadPool::updateCheckpoint() {
    // Find the first odd number greater than the next checkpoint
    uint64_t end = m_checkpoint[0] + CHECKPOINT_SIZE;
    end = end + 1 - (end % 2);

    // This is ok cause theres only 4 values. Avoid branching in a for loop.
    m_checkpoint[3] = m_checkpoint[2];
    m_checkpoint[2] = m_checkpoint[1];
    m_checkpoint[1] = m_checkpoint[0];
    m_checkpoint[0] = min(end, m_endRange);
}

void ThreadPool::waitForWorkers() {
    // TODO
}

bool ThreadPool::doneProcessing() {
    // We need to wait until after we are done processing all up until m_endRange.
    return m_checkpoint[1] == m_endRange;
}
