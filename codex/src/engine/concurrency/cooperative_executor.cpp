#pragma once

#include "public/cooperative_executor.h"

namespace codex::cc {
    CooperativeExecutor::CooperativeExecutor() noexcept
    {
    }

    void CooperativeExecutor::submit(const Job& job)
    {
        auto jobs_queue = jobs_.lock();
        jobs_queue->push(job);
    }

    void CooperativeExecutor::tick()
    {
        std::queue<Job> local;
        {
            auto jobs_queue = jobs_.lock();
            std::swap(*jobs_queue, local);
        }

        while (!local.empty()) {
            auto job = std::move(local.front());
            local.pop();
            job();
        }
    }

    CooperativeExecutor::Awaiter CooperativeExecutor::operator co_await()
    {
        return Awaiter{ *this };
    }
} // namespace codex::cc
