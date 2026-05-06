#pragma once

#include <engine/core/public/common_def.h>
#include <engine/core/public/exception.h>

namespace codex::cc {
    CX_CUSTOM_EXCEPTION(ConcurrencyException, "A Concurrency exception occured.")

    // TODO: Rename to Sync and create Codex Exceptions for CC stuff and handle them here.
    // TODO: Like Sync in mgrd, add debug checking for the same thread trying to lock twice.
    template <typename T>
    class Mutex
    {
    public:
        class ScopedGuard;

    private:
        T                         object_;
        mutable std::mutex        mutex_;
        mutable std::atomic<bool> locked_;
        mutable std::thread::id   owner_thread_;

    public:
        template <typename... TArgs>
        constexpr Mutex(TArgs&&... args)
            : object_(std::forward<TArgs>(args)...)
            , locked_(false)
        {
        }
        inline Mutex(const Mutex<T>& other)               = delete;
        inline Mutex<T>& operator=(const Mutex<T>& other) = delete;
        inline Mutex(Mutex<T>&& other) noexcept
        {
            if (this != &other) {
                const auto guard = other.lock();

                object_ = std::move(other.object_);
            }
        }
        constexpr Mutex<T>& operator=(Mutex<T>&& other) noexcept
        {
            Mutex<T>{ std::move(other) }.swap(*this);
            return *this;
        }
        constexpr ~Mutex() noexcept
        {
            if (is_locked_by_current_thread()) {
                mutex_unlock();
            }
        }

    public:
        [[nodiscard]] constexpr ScopedGuard       value() noexcept { return ScopedGuard(*this); }
        [[nodiscard]] constexpr const ScopedGuard value() const noexcept { return const_cast<Mutex*>(this)->value(); }
        [[nodiscard]] constexpr ScopedGuard       operator->() noexcept { return ScopedGuard(*this); }
        [[nodiscard]] constexpr const ScopedGuard operator->() const noexcept
        {
            return const_cast<Mutex<T>*>(this)->operator->();
        }
        [[nodiscard]] constexpr ScopedGuard       operator*() noexcept { return ScopedGuard(*this); }
        [[nodiscard]] constexpr const ScopedGuard operator*() const noexcept
        {
            return const_cast<Mutex<T>*>(this)->operator*();
        }

    private:
        inline void mutex_lock() const
        {
            if (is_locked_by_current_thread()) {
                throw ConcurrencyException("Same Thread tried locking the same Mutex more than once.");
            } else {
                mutex_.lock();
                owner_thread_ = std::this_thread::get_id();
                locked_.store(true, std::memory_order_release);
            }
        }
        [[nodiscard]] inline bool try_lock() const noexcept
        {
            if (mutex_.try_lock()) {
                owner_thread_ = std::this_thread::get_id();
                locked_.store(true, std::memory_order_release);
                return true;
            }
            return false;
        }
        inline void mutex_unlock() const noexcept
        {
            if (locked_.load(std::memory_order_acquire) && owner_thread_ == std::this_thread::get_id()) {
                locked_.store(false, std::memory_order_release);
                owner_thread_ = std::thread::id{};
                mutex_.unlock();
            }
        }

    public:
        [[nodiscard]] inline bool locked() const noexcept { return locked_.load(std::memory_order_acquire); }
        [[nodiscard]] inline bool is_locked_by_current_thread() const noexcept
        {
            return locked() && owner_thread_ == std::this_thread::get_id();
        }
        [[nodiscard]] inline ScopedGuard       lock() noexcept { return ScopedGuard(*this); }
        [[nodiscard]] inline const ScopedGuard lock() const { return const_cast<Mutex<T>*>(this)->lock(); }
        inline void                            swap(Mutex<T>& other) noexcept
        {
            const auto guard  = lock();
            const auto guard1 = other.lock();

            std::swap(object_, other.object_);
        }

    public:
        template <typename... TArgs>
        [[nodiscard]] static inline Mutex make(TArgs&&... args) noexcept
        {
            return Mutex<T>(std::forward<TArgs>(args)...);
        }
    };

    template <typename T>
    class Mutex<T>::ScopedGuard
    {
        friend class Mutex<T>;

    private:
        Mutex<T>& mutex_;

    private:
        constexpr ScopedGuard(Mutex<T>& mutex)
            : mutex_(mutex)
        {
            mutex_.mutex_lock();
        }

    public:
        constexpr ~ScopedGuard() noexcept { mutex_.mutex_unlock(); }

    public:
        ScopedGuard(const ScopedGuard&) noexcept            = delete;
        ScopedGuard& operator=(const ScopedGuard&) noexcept = delete;
        ScopedGuard(ScopedGuard&&) noexcept                 = delete;
        ScopedGuard& operator=(ScopedGuard&&) noexcept      = delete;

    public:
        [[nodiscard]] constexpr T&       value() noexcept { return mutex_.object_; }
        [[nodiscard]] constexpr const T& value() const noexcept { return const_cast<ScopedGuard*>(this)->value(); }
        [[nodiscard]] constexpr T*       operator->() noexcept { return std::addressof(mutex_.object_); }
        [[nodiscard]] constexpr const T* operator->() const noexcept
        {
            return const_cast<ScopedGuard*>(this)->operator->();
        }
        [[nodiscard]] constexpr T&       operator*() noexcept { return mutex_.object_; }
        [[nodiscard]] constexpr const T& operator*() const noexcept
        {
            return const_cast<ScopedGuard*>(this)->operator*();
        }
    };
} // namespace codex::cc
