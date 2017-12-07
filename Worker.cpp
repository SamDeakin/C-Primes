#include <atomic>

#include "Worker.hpp"
#include "ThreadPool.hpp"

#include <iostream>

Worker::Worker(ThreadPool& parent) :
    m_waiting(false),
    m_parent(parent) {
}

Worker::~Worker() {
}

void Worker::start() {
    m_thread = std::thread(&Worker::doWork, this);
}

void Worker::finish() {
    m_thread.join();
}

void Worker::waitUntilDone() {
    std::unique_lock<std::mutex> lock(m_waitingMutex);
    while(!m_waiting) {
        m_parent.m_poolCV.wait(lock);
    }
}

void Worker::doWork() {
    while (!shouldExit()) {
        // We process our previously constructed queues first to avoid as much waiting time on the work queue as possible
        for (uint64_t num : m_primeSet) {
            process(num);
        }

        // Do after prime set so we don't process anything twice
        while (!m_savedWork.empty()) {
            uint64_t num = m_savedWork.front();
            m_savedWork.pop();

            // Process this number for the next checkpoint and add it to prime set if it is prime
            if (m_parent.isPrime(num)) {
                m_primeSet.push_back(num);
                process(num);
            }
        }

        while (addingWork()) {
            // We essentially busy wait trying to get work each iteration until no more work is being added and the queue is empty
            uint64_t num;
            bool t = m_parent.getWork(num);
            if (t) {
                process(num);
                m_savedWork.push(num);
            }
        }

        waitOnCV();
    }
}

void Worker::process(uint64_t value) {
    // TODO
    std::cout << "p" << value << std::endl;
}

bool Worker::shouldExit() {
    return m_parent.m_threadsShouldExit.load(std::memory_order_acquire);
}

bool Worker::addingWork() {
    bool adding = m_parent.m_addingWork.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);

    bool empty = !m_parent.empty();
    return adding || empty;
}

void Worker::waitOnCV() {
    std::unique_lock<std::mutex> lock(m_waitingMutex);
    m_waiting = true;
    m_parent.m_poolCV.notify_all();
    m_parent.m_workerCV.wait(lock);
    m_waiting = false;
}
