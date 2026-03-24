#pragma once

namespace codex::cc {
    // TODO: Void overload
    template <typename T>
    class Task
    {
    public:
        struct promise_type
        {
            Task get_return_object() { return Task{ std::coroutine_handle<promise_type>::from_promise(*this) }; }

            // Lazy execution, suspend until co_await
            std::suspend_always initial_suspend() { return {}; }

            // Keep coroutine state alive after completion
            std::suspend_always final_suspend() noexcept { return {}; }

            // Handle co_return, just forward the value to result_.
            template <typename U>
            void return_value(U&& value)
            {
                result_ = std::forward<U>(value);
            }

            // Handle unhandled exceptions.
            void unhandled_exception() { exception_ = std::current_exception(); }

        private:
            std::optional<T>   result_;
            std::exception_ptr exception_;
        };
        class awaiter
        {
        public:
            explicit(false) awaiter(const bool ready)
                : ready_{ ready }
            {
            }

        public:
            bool        await_ready() const noexcept { return ready_; }
            static void await_suspend(std::coroutine_handle<>) noexcept {}
            static void await_resume() noexcept {}

        private:
            bool ready_;
        };

    public:
        explicit Task(std::coroutine_handle<promise_type> handle)
            : handle_(handle)
        {
        }
        Task(const Task&) = delete;
        Task(Task&& other) noexcept
            : handle_{ other.handle_ }
        {
            other.handle_ = nullptr;
        }
        ~Task() noexcept
        {
            if (handle_) {
                handle_.destroy();
            }
        }

    public:
        Task& operator=(const Task&) = delete;
        Task& operator=(Task&& other) noexcept { return Task{ std::move(other) }.swap(*this); }

    public:
        explicit operator bool()
        {
            fill();
            return !handle_.done();
        }
        T operator()()
        {
            fill();
            full_ = false;
            return std::move(handle_.promise().result_);
        }

    public:
        Task& swap(Task& other) noexcept
        {
            std::swap(handle_, other.handle_);
            return *this;
        }

    private:
        void fill()
        {
            if (!full_) {
                handle_();
                if (handle_.promise().exception_) {
                    std::rethrow_exception(handle_.promise().exception_);
                    full_ = true;
                }
            }
        }

    private:
        bool                                full_ = false;
        std::coroutine_handle<promise_type> handle_;
    };
} // namespace codex::cc
