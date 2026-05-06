#pragma once

#include <engine/core/public/exception.h>

#include <optional>

namespace codex::cc {
    CX_CUSTOM_EXCEPTION(TaskException, "Unknown task exception occured.")

    using Job = std::function<void()>;

    namespace detail {
        template <typename T>
        struct result_storage
        {
            std::optional<T> value_;

            template <typename U>
            void return_value(U&& v)
            {
                value_.emplace(std::forward<U>(v));
            }

            T get()
            {
                CX_ASSERT(value_.has_value(), "Task result consumed without a co_return value.");
                return std::move(*value_);
            }
        };

        template <>
        struct result_storage<void>
        {
            void return_void() noexcept {}
        };
    } // namespace detail

    template <typename T>
    class Task
    {
    public:
        struct promise_type : public detail::result_storage<T>
        {
            friend class Task<T>;

        public:
            static constexpr u32 kRunning   = 0;
            static constexpr u32 kCompleted = 1;
            static constexpr u32 kAwaiting  = 2;

            struct FinalAwaiter
            {
                bool await_ready() const noexcept { return false; }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
                {
                    h.promise().sync_.release();
                    auto prev = h.promise().state_.exchange(kCompleted, std::memory_order_acq_rel);
                    auto cont = h.promise().continuation_;
                    h.promise().release_ref();

                    if (prev == kAwaiting)
                        return cont;

                    return std::noop_coroutine();
                }

                void await_resume() noexcept {}
            };

        public:
            promise_type() noexcept {}

        public:
            Task get_return_object() { return Task{ std::coroutine_handle<promise_type>::from_promise(*this) }; }

            std::suspend_never initial_suspend()
            {
                acquire_ref();
                return {};
            }

            FinalAwaiter final_suspend() noexcept { return {}; }

            void unhandled_exception() noexcept { exception_ = std::current_exception(); }

        private:
            void release_ref() noexcept
            {
                if (ref_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    auto h = std::coroutine_handle<promise_type>::from_promise(*this);
                    h.destroy();
                }
            }
            void acquire_ref() noexcept { ref_.fetch_add(1, std::memory_order_acq_rel); }

        private:
            std::exception_ptr      exception_ = nullptr;
            std::binary_semaphore   sync_{ 0 };
            std::coroutine_handle<> continuation_ = {};
            std::atomic<u32>        ref_          = 0;
            std::atomic<u32>        state_        = kRunning;
        };

        struct Awaiter
        {
            bool await_ready()
            {
                return handle_.promise().state_.load(std::memory_order_acquire) == promise_type::kCompleted;
            }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont)
            {
                handle_.promise().continuation_ = cont;
                auto prev =
                    handle_.promise().state_.exchange(promise_type::kAwaiting, std::memory_order_acq_rel);

                if (prev == promise_type::kCompleted)
                    return cont;

                return std::noop_coroutine();
            }

            T await_resume() requires(!std::is_void_v<T>)
            {
                if (handle_.promise().exception_)
                    std::rethrow_exception(handle_.promise().exception_);

                return handle_.promise().get();
            }

            void await_resume() requires(std::is_void_v<T>)
            {
                if (handle_.promise().exception_)
                    std::rethrow_exception(handle_.promise().exception_);
            }

        public:
            std::coroutine_handle<promise_type> handle_;
        };

    public:
        explicit(false) Task(std::coroutine_handle<promise_type> handle)
            : handle_{ handle }
        {
            handle_.promise().acquire_ref();
        }

        Task(const Task&) = delete;

        Task(Task&& other) noexcept
            : handle_{ std::exchange(other.handle_, nullptr) }
        {
        }

        ~Task() noexcept
        {
            if (handle_) {
                // info("~Task<T>::Task(): Ref: {}", handle_.promise().ref_.load(std::memory_order_acquire));
                handle_.promise().release_ref();
            }
        }

    public:
        Task& operator=(const Task&) = delete;

        Task& operator=(Task&& other) noexcept { return Task{ std::move(other) }.swap(*this); }

    public:
        explicit operator bool() { return !handle_.done(); }

        Awaiter operator co_await() { return Awaiter{ handle_ }; }

        T await_sync() requires(!std::is_void_v<T>)
        {
            if (!handle_)
                throw std::runtime_error("null task");

            if (!handle_.done())
                handle_.promise().sync_.acquire();

            if (handle_.promise().exception_)
                std::rethrow_exception(handle_.promise().exception_);

            return handle_.promise().get();
        }

        void await_sync() requires(std::is_void_v<T>)
        {
            if (!handle_)
                throw std::runtime_error("null task");

            if (!handle_.done())
                handle_.promise().sync_.acquire();

            if (handle_.promise().exception_)
                std::rethrow_exception(handle_.promise().exception_);
        }

        Task& resume()
        {
            if (handle_ && !handle_.done()) {
                u32 expected = 1;
                handle_.promise().ref_.compare_exchange_strong(expected, 2, std::memory_order_acq_rel);
                handle_.resume();
            }

            return *this;
        }

        [[nodiscard]] inline bool done() const noexcept { return handle_.done(); }

    public:
        Task& swap(Task& other) noexcept
        {
            std::swap(handle_, other.handle_);
            return *this;
        }

    private:
        std::coroutine_handle<promise_type> handle_ = {};
    };
} // namespace codex::cc
