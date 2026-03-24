#pragma once

#include "thread_pool.h"

namespace codex::cc {
    struct IExecutor
    {
        virtual void schedule(Job) = 0;
    };

    struct ThreadPoolExecutor : public IExecutor
    {
        ThreadPoolExecutor(ThreadPool& pool)
            : pool_{ pool }
        {
        }

    public:
        void schedule(Job job) override { pool_.enqueue(job); }

    private:
        ThreadPool& pool_;
    };
} // namespace codex::cc
