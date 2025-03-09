#pragma once
#include <iostream>
#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iomanip>
#include <tuple>

// 同步时钟类型（假设定义）
class SyncClock;

// 日志策略枚举
enum class LogStrategy {
    DEFAULT_FORMAT  // 目前仅支持默认格式
};

class AsyncLogger {
public:
    AsyncLogger() 
        : m_filename("async_performance_log.txt"),
          m_running(true),
          m_worker(&AsyncLogger::logWorker, this)
    {
        m_file.open(m_filename);
        if (m_file.is_open()) {
            m_file << std::fixed << std::setprecision(6);
        }
    }

    ~AsyncLogger() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_cv.notify_one();
        m_worker.join();
        
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    void commit(LogStrategy strategy, const std::string& caller, const SyncClock& clock);

private:
    void logWorker();

    std::ofstream m_file;
    const std::string m_filename;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::tuple<LogStrategy, std::string, double, double, double, double>> m_queue;
    
    std::atomic<bool> m_running;
    std::thread m_worker;
};