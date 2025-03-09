#include "../inc/logger.h"
#include "../inc/clock.h"

void AsyncLogger::commit(LogStrategy strategy, const std::string& caller, const SyncClock& clock) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(strategy, caller, 
                        clock.systemTime, clock.curPts, clock.diff, clock.tailPts);
    }
    m_cv.notify_one();
}

void AsyncLogger::logWorker() {
    while (true) {
        std::tuple<LogStrategy, std::string, double, double, double, double> entry;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]{
                return !m_queue.empty() || !m_running;
            });

            if (!m_running && m_queue.empty()) break;

            if (!m_queue.empty()) {
                entry = m_queue.front();
                m_queue.pop();
            }
        }

        if (m_file.is_open()) {
            auto [strategy, caller, t1, t2, t3, t4] = entry;
            
            // 目前仅处理默认格式
            if (strategy == LogStrategy::DEFAULT_FORMAT) {
                m_file << "========== BEGIN ENTRY ==========\n"
                        << "Caller    = " << caller << "\n"
                        << "curTime  = " << t1 << "\n"
                        << "curPts   = " << t2 << "\n"
                        << "diff     = " << t3 << "\n"
                        << "tailPts = " << t4 << "\n"  // 修正原代码中的语法错误
                        << "=========== END ENTRY ===========\n\n";
            }
        }
    }

    // 确保队列清空
    while (!m_queue.empty()) {
        auto entry = m_queue.front();
        m_queue.pop();
        if (m_file.is_open()) {
            auto [strategy, caller, t1, t2, t3, t4] = entry;
            if (strategy == LogStrategy::DEFAULT_FORMAT) {
                m_file << "========== BEGIN ENTRY ==========\n"
                        << "Caller    = " << caller << "\n"
                        << "curTime  = " << t1 << "\n"
                        << "curPts   = " << t2 << "\n"
                        << "diff     = " << t3 << "\n"
                        << "tailPts = " << t4 << "\n"
                        << "=========== END ENTRY ===========\n\n";
            }
        }
    }
}