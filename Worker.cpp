#include "Worker.hpp"

Worker::Worker(ThreadPool& parent) :
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

void Worker::doWork() {

}
