#pragma once

#include <memory>

#include "ref.h"

namespace codex {
    template <typename T>
    class Shared
    {
        template <typename U>
        friend class Shared;

        template <typename U>
        friend class Ref;

    public:
        constexpr Shared() noexcept = default;
        constexpr Shared(std::nullptr_t) noexcept
            : impl_(nullptr)
        {
        }
        explicit Shared(T* ptr)
            requires(!std::is_void_v<T>)
            : impl_(ptr)
        {
        }
        template <typename Deleter>
            requires(!std::is_void_v<T> && std::is_invocable_v<Deleter, T*>)
        Shared(T* ptr, Deleter d)
            : impl_(ptr, std::move(d))
        {
        }
        // Implicit conversion from std::shared_ptr — needed for std::shared_from_this() interop.
        Shared(std::shared_ptr<T> sp) noexcept
            : impl_(std::move(sp))
        {
        }
        Shared(const Shared&) noexcept            = default;
        Shared(Shared&&) noexcept                 = default;
        Shared& operator=(const Shared&) noexcept = default;
        Shared& operator=(Shared&&) noexcept      = default;
        ~Shared()                                 = default;

        template <typename U>
            requires(std::is_convertible_v<U*, T*>)
        Shared(const Shared<U>& other) noexcept
            : impl_(other.impl_)
        {
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*>)
        Shared(Shared<U>&& other) noexcept
            : impl_(std::move(other.impl_))
        {
        }

    public:
        [[nodiscard]] T*       get() noexcept { return impl_.get(); }
        [[nodiscard]] const T* get() const noexcept { return impl_.get(); }

        [[nodiscard]] operator bool() const noexcept { return impl_ != nullptr; }
                      operator std::shared_ptr<T>() const noexcept { return impl_; }

        T* operator->() noexcept
            requires(!std::is_void_v<T>)
        {
            return impl_.get();
        }
        const T* operator->() const noexcept
            requires(!std::is_void_v<T>)
        {
            return impl_.get();
        }

        // Use a non-void stand-in for the return type so the declaration is valid even when
        // T=void, Clang checks the return type before evaluating requires-clauses.
        using deref_t = std::conditional_t<std::is_void_v<T>, std::byte, T>;

        deref_t& operator*() noexcept
            requires(!std::is_void_v<T>)
        {
            return *impl_;
        }
        const deref_t& operator*() const noexcept
            requires(!std::is_void_v<T>)
        {
            return *impl_;
        }

        Shared& operator=(std::nullptr_t) noexcept
        {
            impl_ = nullptr;
            return *this;
        }

    public:
        void reset(T* ptr = nullptr) { impl_.reset(ptr); }
        void swap(Shared& other) noexcept { impl_.swap(other.impl_); }

        [[nodiscard]] Ref<T>       as_ref() noexcept { return Ref<T>{ *this }; }
        [[nodiscard]] Ref<const T> as_ref() const noexcept { return Ref<const T>{ *this }; }

        // Static cast to a different type — analogous to std::static_pointer_cast.
        template <typename U>
        [[nodiscard]] Shared<U> as() const noexcept
        {
            return Shared<U>{ std::static_pointer_cast<U>(impl_) };
        }

    public:
        // Call new directly so friend declarations on T granting Box<T>/Shared<T> access
        // allow construction of types with private/protected ctors.
        // Additionally, make_shared cannot be used here because it performs the allocation
        // inside its own scope where friendship with T does not apply.
        template <typename... Args>
            requires(!std::is_void_v<T>)
        [[nodiscard]] static Shared make(Args&&... args)
        {
            return Shared{ new T(std::forward<Args>(args)...) };
        }

        [[nodiscard]] static Shared from(T* ptr)
            requires(!std::is_void_v<T>)
        {
            return Shared{ ptr };
        }

    public:
        bool operator==(std::nullptr_t) const noexcept { return impl_ == nullptr; }
        bool operator!=(std::nullptr_t) const noexcept { return impl_ != nullptr; }
        bool operator==(const Shared& other) const noexcept { return impl_ == other.impl_; }

    private:
        std::shared_ptr<T> impl_;
    };

    template <typename T>
    Ref<T>::Ref(const Shared<T>& s) noexcept
        : impl_(s.impl_)
    {
    }

    template <typename T>
    Ref<T>& Ref<T>::operator=(const Shared<T>& s) noexcept
    {
        impl_ = s.impl_;
        return *this;
    }

    template <typename T>
    template <typename U>
        requires(std::is_convertible_v<U*, T*>)
    Ref<T>::Ref(const Shared<U>& other) noexcept
        : impl_(other.impl_)
    {
    }

    template <typename T>
    Shared<T> Ref<T>::lock() const
    {
        auto sp = impl_.lock();
        if (!sp)
            throw ExpiredRefException{};
        return Shared<T>{ std::move(sp) };
    }
} // namespace codex
