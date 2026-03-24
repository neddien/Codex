#pragma once

#include "task.h"

#include "mutex.h"

namespace codex::cc {
    class CooperativeExecutor
    {
    private:
        struct Awaiter
        {
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) const { exec.schedule(h); }
            void await_resume() const noexcept {}

        public:
            CooperativeExecutor& exec;
        };

    public:
        CooperativeExecutor() noexcept;

    public:
        void schedule(const Job& job);
        void tick();

    public:
        Awaiter operator co_await();

    private:
        Mutex<std::queue<Job>> jobs_;
    };
} // namespace codex::cc
