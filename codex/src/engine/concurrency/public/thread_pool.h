#pragma once

#include <condition_variable>

#include "task.h"

#include <engine/core/public/log.h>

namespace codex::cc {
    // TODO: Expand to a Worker Pool with Priorities, multiple job queues, work stealing
    // Group worker jobs: Physics, Render, AI, FS etc...
    class ThreadPool : public Loggable<"ThreadPool">
    {
    public:
        ThreadPool(const u32 initial_count) noexcept;
        ~ThreadPool() noexcept;

    public:
        [[nodiscard]] u32 available_concurrency() const noexcept { return threads_.size() - jobs_.size(); }

    public:
        void enqueue(const Job& job) noexcept;

    private:
        void alloc(const u32 count) noexcept;

    private:
        std::vector<std::thread> threads_;
        std::queue<Job>          jobs_;
        std::mutex               mutex_;
        std::condition_variable  cv_;
        bool                     stop_ = false;
    };
} // namespace codex::cc
