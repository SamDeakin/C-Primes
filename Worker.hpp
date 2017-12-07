#ifndef __WORKER_HPP__
#define __WORKER_HPP__

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
private:
    // The main running loop that will be performed on m_thread
    // Communicates with m_parent to get work and return results
    void doWork();

    // Holds a reference to the parent ThreadPool to communicate with
    ThreadPool& m_parent;

    // Will perform all work on this thread
    std::thread m_thread;
};

#endif // __WORKER_HPP__
