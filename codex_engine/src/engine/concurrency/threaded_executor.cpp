#include "public/threaded_executor.h"

namespace codex::cc {
    ThreadedExecutor::ThreadedExecutor(cc::ThreadPool& pool)
        : thread_pool_{ pool }
    {
    }

    void ThreadedExecutor::schedule(const Job& job)
    {
        thread_pool_.enqueue(job);
    }

    ThreadedExecutor::Awaiter ThreadedExecutor::operator co_await()
    {
        return Awaiter{ *this };
    }
} // namespace codex::cc
