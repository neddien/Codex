#pragma once

namespace codex::cc {
    using Job = std::function<void()>;

    // TODO: Expand to a Worker Pool with Priorities, multiple job queues, work stealing
    // Group worker jobs: Physics, Render, AI, FS etc...
    class ThreadPool
    {
    public:
        ThreadPool(const u32 initial_count = std::thread::hardware_concurrency()) noexcept;
        ~ThreadPool() noexcept;

    public:
        void enqueue(Job& job) noexcept;

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
