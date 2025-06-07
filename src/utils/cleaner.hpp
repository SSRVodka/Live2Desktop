#pragma once

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <vector>

#include "utils/logger.h"

// 主进程正常退出时需要手动执行 execute_cleanup 来执行已注册的回收函数。意外退出（信号）则 Cleaner 会自动调用。
class Cleaner {
public:

    // buffer 用于展示函数指针地址，方便调试
    static constexpr size_t FUNC_PTR_BUFSIZE = 1024;

    static Cleaner& instance() {
        static Cleaner global_cleaner;
        return global_cleaner;
    }

    // 注册回收函数
    void register_cleanup(std::function<void()> func);

    // 执行所有注册的清理函数
    void execute_cleanup();

    // 禁止拷贝和移动
    Cleaner(const Cleaner&) = delete;
    Cleaner& operator=(const Cleaner&) = delete;

private:
    std::vector<std::function<void()>> cleanup_functions_;
    std::mutex cleanup_mutex_;
    std::atomic<bool> cleaning_{false};

    Cleaner();

    ~Cleaner();
};
