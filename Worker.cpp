#include <atomic>

#include "Worker.hpp"
#include "ThreadPool.hpp"

Worker::Worker(ThreadPool& parent) :
    m_start(false),
    m_waiting(false),
    m_exit(false),
    m_parent(parent) {
}

Worker::~Worker() {
}

void Worker::start() {
    m_thread = std::thread(&Worker::doWork, this);
}

void Worker::finish() {
    {
        std::unique_lock<std::mutex> lock(m_waitingMutex);
        if (!m_exit) {
            if (m_waiting) {
                m_parent.m_workerCV.notify_all();
            } else {
                m_parent.m_poolCV.wait(lock);
                m_parent.m_workerCV.notify_all();
            }
        }
    }
    m_thread.join();
}

void Worker::waitUntilDone() {
    std::unique_lock<std::mutex> lock(m_waitingMutex);
    while(!m_waiting || m_start) {
        m_parent.m_poolCV.wait(lock);
    }
    m_start = true;
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
    {
        // Signal that the thread is exiting.
        std::lock_guard<std::mutex> lock(m_waitingMutex);
        m_exit = true;
        m_parent.m_poolCV.notify_all();
    }
}

void Worker::process(uint64_t value) {
    uint64_t start_range = m_parent.m_checkpoint[1];
    uint64_t end_range = m_parent.m_checkpoint[0];

    // Increment by this amount to skip even multiples
    uint64_t interval = 2 * value;

    // We calculate the start to be the first odd multiple of value >= start_range
    uint64_t multiple = start_range / value;
    if (start_range % value != 0) {
        multiple++;
    }
    multiple += 1 - (multiple % 2); // Adjust to be next odd number

    // We don't want to process value ever
    if (multiple == 1) {
        multiple += 2;
    }

    for (uint64_t i = multiple * value; i < end_range; i = i + interval) {
        m_parent.setResult(i);
    }
}

bool Worker::shouldExit() {
    bool p = m_parent.m_threadsShouldExit.load(std::memory_order_acquire);
    return p;
}

bool Worker::addingWork() {
    bool adding = m_parent.m_addingWork.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    return adding || !m_parent.empty();
}

void Worker::waitOnCV() {
    std::unique_lock<std::mutex> lock(m_waitingMutex);
    m_waiting = true;
    m_parent.m_poolCV.notify_all();
    m_parent.m_workerCV.wait(lock);
    m_waiting = false;
    m_start = false;
}
