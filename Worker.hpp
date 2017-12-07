#ifndef __WORKER_HPP__
#define __WORKER_HPP__

#include <list>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool;

class Worker {
public:
    // Create a Worker object
    Worker(ThreadPool& parent);
    ~Worker();

    // The thread will not start running until this is called
    void start();

    // Tell the thread to finish running and return. Pretty much just calls thread.join
    void finish();

    // The calling thread waits on the parents m_poolCV until this worker is done all it's work
    void waitUntilDone();
private:
    // The main running loop that will be performed on m_thread
    // Communicates with m_parent to get work and return results
    void doWork();

    // Process the number value
    void process(uint64_t value);

    // Returns true if the program is ending and this worker should wrap up and exit
    bool shouldExit();

    // Returns true if the worker should keep checking for work
    bool addingWork();

    // Wait on the parents m_workerCV
    void waitOnCV();

    // Holds a reference to the parent ThreadPool to communicate with
    ThreadPool& m_parent;

    // Will perform all work on this thread
    std::thread m_thread;

    // A list of prime numbers we have found
    std::list<uint64_t> m_primeSet;

    // A list of numbers we must check for primeness
    std::queue<uint64_t> m_savedWork;

    // True if this thread is waiting
    bool m_waiting;

    // A thread must acquire this lock to check m_waiting
    std::mutex m_waitingMutex;
};

#endif // __WORKER_HPP__
