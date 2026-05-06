#pragma once

#include "thread_pool.h"

namespace codex::cc {
    class ThreadedExecutor
    {
    private:
        struct Awaiter
        {
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) const { exec.submit(h); }
            void await_resume() const noexcept {}

        public:
            ThreadedExecutor& exec;
        };

    public:
        ThreadedExecutor(ThreadPool& pool);

    public:
        void submit(const Job& job);

    public:
        Awaiter operator co_await();

    private:
        ThreadPool& thread_pool_;
    };
} // namespace codex::cc
