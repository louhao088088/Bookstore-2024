#pragma once 
#include "MemoryRiver.hpp"
class AsyncLogger {
private:
    queue<string> logQueue;
    mutex mtx;
    condition_variable cv;
    thread worker;
    bool running = true;

    void processLogs() {
        ofstream logfile(OPERATION_LOG, ios::app);
        while (running || !logQueue.empty()) {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this] { return !logQueue.empty() || !running; });
            while (!logQueue.empty()) {
                logfile << logQueue.front() << endl;
                logQueue.pop();
            }
        }
    }

public:
    AsyncLogger() : worker(&AsyncLogger::processLogs, this) {}

    ~AsyncLogger() {
        running = false;
        cv.notify_all();
        worker.join();
    }

    void log(const string& message) {
        unique_lock<mutex> lock(mtx);
        logQueue.push(message);
        cv.notify_one();
    }
};