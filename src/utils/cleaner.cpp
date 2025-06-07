
#include "utils/cleaner.hpp"

void Cleaner::register_cleanup(std::function<void()> func) {
    char func_ptr_buf[FUNC_PTR_BUFSIZE];
    std::lock_guard<std::mutex> lock(cleanup_mutex_);
    if (!cleaning_.load()) {
        cleanup_functions_.push_back(std::move(func));
        snprintf(func_ptr_buf, Cleaner::FUNC_PTR_BUFSIZE, "%p", (void*)(&func));
        stdLogger.Info(std::string("registed cleanup function ") + func_ptr_buf);
    } else {
        // 如果正在清理过程中注册，立即执行该函数
        func();
    }
}

Cleaner::Cleaner() {
    auto signal_handler = [](int sig) {
        Cleaner::instance().execute_cleanup();
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    };

    std::signal(SIGINT, signal_handler);  // Ctrl+C
    std::signal(SIGTERM, signal_handler); // kill 命令
    std::signal(SIGABRT, signal_handler); // abort() 调用
}

Cleaner::~Cleaner() {
    execute_cleanup();
}

void Cleaner::execute_cleanup() {
    char func_ptr_buf[FUNC_PTR_BUFSIZE];

    // 设置清理标志，防止重复执行
    if (cleaning_.exchange(true)) return;

    stdLogger.Info("trigger cleaning up...");

    std::vector<std::function<void()>> functions_to_run;
    {
        std::lock_guard<std::mutex> lock(cleanup_mutex_);
        functions_to_run = std::move(cleanup_functions_);
    }

    // 按注册顺序的逆序执行（后注册的先执行）
    auto it = functions_to_run.rbegin();
    for (; it != functions_to_run.rend(); ++it) {
        try {
            snprintf(func_ptr_buf, Cleaner::FUNC_PTR_BUFSIZE, "%p", (void*)&(*it));
            stdLogger.Info(std::string("executing cleanup function ") + func_ptr_buf);
            (*it)();
        } catch (std::exception &ex) {
            // 忽略所有异常，确保所有函数都能被执行
            snprintf(func_ptr_buf, Cleaner::FUNC_PTR_BUFSIZE, "%p", (void*)&(*it));
            stdLogger.Warning(std::string("cleanup function ") + func_ptr_buf
                + " raise error during executing: " + ex.what());
        }
    }
    stdLogger.Info("finish cleaning up. Bye :)");
}

