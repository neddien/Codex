#pragma once

#include <memory>

namespace codex {
    template <typename T, typename Deleter = std::default_delete<T>>
    class Box
    {
        template <typename U, typename D>
        friend class Box;

    public:
        Box() = default;
        constexpr Box(std::nullptr_t) noexcept
            : impl_(nullptr)
        {
        }
        explicit Box(T* ptr) noexcept
            : impl_(ptr)
        {
        }
        Box(T* ptr, Deleter d) noexcept
            : impl_(ptr, std::move(d))
        {
        }
        Box(Box&&) noexcept            = default;
        Box& operator=(Box&&) noexcept = default;
        ~Box()                         = default;

        template <typename U, typename D>
            requires(std::is_convertible_v<U*, T*>)
        Box(Box<U, D>&& other) noexcept
            : impl_(std::move(other.impl_))
        {
        }

    public:
        [[nodiscard]] T*             get() noexcept { return impl_.get(); }
        [[nodiscard]] const T*       get() const noexcept { return impl_.get(); }
        [[nodiscard]] Deleter&       deleter() noexcept { return impl_.get_deleter(); }
        [[nodiscard]] const Deleter& deleter() const noexcept { return impl_.get_deleter(); }

    public:
        [[nodiscard]]          operator bool() const noexcept { return impl_ != nullptr; }
        [[nodiscard]] T*       operator->() noexcept { return impl_.get(); }
        [[nodiscard]] const T* operator->() const noexcept { return impl_.get(); }
        [[nodiscard]] T&       operator*() noexcept { return *impl_; }
        [[nodiscard]] const T& operator*() const noexcept { return *impl_; }

        Box& operator=(std::nullptr_t) noexcept
        {
            impl_ = nullptr;
            return *this;
        }

    public:
        void reset(T* ptr = nullptr) noexcept { impl_.reset(ptr); }
        T*   release() noexcept { return impl_.release(); }
        Box& swap(Box& other) noexcept
        {
            impl_.swap(other.impl_);
            return *this;
        }

    public:
        // Consumes this Box and reinterprets it as Box<U> via static_cast.
        template <typename U>
        [[nodiscard]] Box<U> as() && noexcept
        {
            return Box<U>{ static_cast<U*>(impl_.release()) };
        }

    public:
        template <typename... Args>
        [[nodiscard]] static Box make(Args&&... args)
        {
            // Call new directly (not make_unique) so that friend declarations on T
            // granting access to Box<T> allow construction of types with private/protected ctors.
            return Box{ new T(std::forward<Args>(args)...) };
        }

        [[nodiscard]] static Box from(T* ptr, Deleter d = {}) noexcept { return Box{ ptr, std::move(d) }; }

    public:
        [[nodiscard]] bool operator==(std::nullptr_t) const noexcept { return impl_ == nullptr; }
        [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept { return impl_ != nullptr; }

    private:
        std::unique_ptr<T, Deleter> impl_;
    };
} // namespace codex
