#include "ThreadPool.hpp"

#include <iostream>

static const uint64_t CHECKPOINT_SIZE = 20; // TODO This number is good for testing but should be larger or configurable

ThreadPool::ThreadPool(uint64_t start, uint64_t end, std::size_t numThreads) :
    m_addingWork(false),
    m_threadsShouldExit(false),
    m_checkpoint{start, start, start, start},
    m_resultsTable(nullptr),
    m_nonPrime(nullptr),
    m_startRange(start),
    m_endRange(end) {
    // Create Workers
    for (int i = 0; i < numThreads; i++) {
        m_workers.push_back(std::make_unique<Worker>(*this));
    }

    // For our base case don't process below 3
    if (start < 3) {
        m_checkpoint[0] = 3;
        m_checkpoint[1] = 3;
        m_checkpoint[2] = 3;
        m_checkpoint[3] = 3;

        // This algorithm won't find 2, but we know to include it here.
        std::cout << 2 << std::endl;
    }
}

ThreadPool::~ThreadPool() {
    m_workers.clear();
}

void ThreadPool::start() {
    // Update tables to start state
    m_resultsTable = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();
    m_nonPrime = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();

    auto toBePrinted = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();

    // Update checkpoints for start
    updateCheckpoint();

    // Start the workers running
    for (auto& worker : m_workers) {
        worker->start();
    }

    // To account for offset starts we must queue every odd number between 3 and the start value / 3
    uint64_t last = m_checkpoint[1] / 3 + 1;
    for (uint64_t i = 3; i < last; i = i + 2) {
        addWork(i);
    }

    while (!doneProcessing()) {
        // Signal workers to wake up and begin processing
        m_workerCV.notify_all();

        // The last number we need to process is exactly checkpoint / 3 so cut off at 1 over to account for int rounding
        uint64_t checkpointEnd = m_checkpoint[0] / 3 + 1;
        uint64_t endOfLast = m_checkpoint[1] / 3 + 1;
        for (
            // Start at first odd number after end of last checkpoint
            uint64_t i = endOfLast + ((endOfLast + 1) % 2);
            i < checkpointEnd; // Iterate over every number less than sqrt(checkpoint)
            i = i + 2 // Iterate over odd numbers
            ) {
            addWork(i);
        }

        std::atomic_thread_fence(std::memory_order_release);
        m_addingWork.store(false, std::memory_order_relaxed);

        // drain toBePrinted
        drainResults(toBePrinted, m_checkpoint[3], m_checkpoint[2]);

        // wait for Workers
        waitForWorkers();

        // update checkpoint
        updateCheckpoint();

        auto newTable = std::make_unique<tbb::concurrent_unordered_set<uint64_t>>();
        toBePrinted = swapResults(std::move(newTable));

        // Set that work is being added again
        std::atomic_thread_fence(std::memory_order_release);
        m_addingWork.store(true, std::memory_order_relaxed);
    }

    // set done all
    m_addingWork.store(false, std::memory_order_release);
    m_threadsShouldExit.store(true, std::memory_order_release);

    // signal Workers to wake up and begin processing. They will see that they should exit and join up.
    m_workerCV.notify_all();

    // drain toBePrinted
    drainResults(toBePrinted, m_checkpoint[3], m_checkpoint[2]);
    // drain m_nonPrime
    drainResults(m_nonPrime, m_checkpoint[2], m_checkpoint[1]);
    // m_resultsTable will not have any new work here, so we do not need to drain it.

    // Signal all workers to wake up. They will see that they should exit.
    m_workerCV.notify_all();

    // Wait for them to exit.
    for (auto& worker : m_workers) {
        worker->finish();
    }
}

bool ThreadPool::getWork(uint64_t& out) {
    return m_workQueue.try_pop(out);
}

bool ThreadPool::empty() {
    return m_workQueue.empty();
}

void ThreadPool::setResult(uint64_t value) {
    m_resultsTable->insert(value);
}

bool ThreadPool::isPrime(uint64_t value) {
    return !m_nonPrime->count(value);
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
    m_checkpoint[0] = std::min(end, m_endRange);
}

void ThreadPool::waitForWorkers() {
    for (auto& worker : m_workers) {
        worker->waitUntilDone();
    }
}

bool ThreadPool::doneProcessing() {
    // We need to wait until after we are done processing all up until m_endRange.
    return m_checkpoint[1] == m_endRange;
}

void ThreadPool::drainResults(const std::unique_ptr<tbb::concurrent_unordered_set<uint64_t>>& table, uint64_t start_range, uint64_t end_range) {

    for (
        // Start at the first odd number greater than start_range.
        uint64_t i = start_range + ((start_range + 1) % 2);
        i < end_range;
        i = i + 2) {
        if (!table->count(i)) {
            std::cout << i << std::endl;
        }
    }
}
