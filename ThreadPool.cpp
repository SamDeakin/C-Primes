#include "ThreadPool.hpp"

static const uint64_t CHECKPOINT_SIZE = 20; // TODO This number is good for testing but should be larger or configurable

ThreadPool::ThreadPool(uint64_t start, uint64_t end, std::size_t numThreads) {

}

ThreadPool::~ThreadPool() {

}

void ThreadPool::start() {
    // TODO After worker API
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

void ThreadPool::updateCheckpoints() {
    // Find the first odd number greater than the last checkpoint
    uint64_t end_num = m_checkpoint[0] + CHECKPOINT_SIZE;
    end_num = end_num + 1 - (end_num % 2);

    // This is ok cause theres only 4 values. Avoid branching in a for loop.
    this->checkpoint[3] = this->checkpoint[2];
    this->checkpoint[2] = this->checkpoint[1];
    this->checkpoint[1] = this->checkpoint[0];
    this->checkpoint[0] = min(end_num, this->max_range);
}
