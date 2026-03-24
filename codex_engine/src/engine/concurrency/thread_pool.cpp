#include "public/thread_pool.h"

namespace codex::cc {
    ThreadPool::ThreadPool(const u32 initial_count) noexcept
    {
        alloc(initial_count);
    }

    ThreadPool::~ThreadPool() noexcept
    {
        {
            std::unique_lock<std::mutex> lock{ mutex_ };
            stop_ = true;
        }
        cv_.notify_all();

        for (auto& thread : threads_)
            thread.join();

        info("ThreadPool: Deallocated");
    }

    void ThreadPool::enqueue(const Job& job) noexcept
    {
        {
            std::unique_lock<std::mutex> lock{ mutex_ };
            jobs_.emplace(std::move(job));
        }
        cv_.notify_one();

        codex::info("ThreadPool: Job enqueued");
    }

    void ThreadPool::alloc(const u32 count) noexcept
    {
        static const auto thread_fn = [this]
        {
            codex::info("ThreadPool: Thread #{} started", std::hash<std::thread::id>{}(std::this_thread::get_id()));

            Job job;
            while (true) {
                codex::info("ThreadPool: Thread #{} goes to sleep",
                            std::hash<std::thread::id>{}(std::this_thread::get_id()));

                {
                    std::unique_lock<std::mutex> lock{ mutex_ };
                    cv_.wait(lock, [this] { return !jobs_.empty() || stop_; });

                    if (stop_ && jobs_.empty()) {
                        codex::info("ThreadPool: Thread #{} exited",
                                    std::hash<std::thread::id>{}(std::this_thread::get_id()));
                        return;
                    }

                    job = std::move(jobs_.front());
                    jobs_.pop();
                }

                codex::info("ThreadPool: Thread #{} picked up Job {}",
                            std::hash<std::thread::id>{}(std::this_thread::get_id()), job.target_type().name());
                job();
            }
        };

        for (u32 i = 0; i < count; ++i) {
            threads_.emplace_back(thread_fn);
        }

        info("ThreadPool: Allocated {} threads", count);
    }
} // namespace codex::cc
