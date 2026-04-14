#pragma once

#include <memory>

#include <engine/core/public/exception.h>

namespace codex {
    template <typename T>
    class Shared;

    CX_CUSTOM_EXCEPTION(ExpiredRefException, "Weak reference is expired.")

    template <typename T>
    class Ref
    {
        template <typename U>
        friend class Ref;

        template <typename U>
        friend class Shared;

    public:
        constexpr Ref() noexcept = default;
        constexpr Ref(std::nullptr_t) noexcept {}

        Ref(const Shared<T>& s) noexcept;
        Ref(const Ref&) noexcept            = default;
        Ref(Ref&&) noexcept                 = default;
        Ref& operator=(const Ref&) noexcept = default;
        Ref& operator=(Ref&&) noexcept      = default;
        Ref& operator=(const Shared<T>& s) noexcept;
        ~Ref() = default;

        template <typename U>
            requires(std::is_convertible_v<U*, T*>)
        Ref(const Ref<U>& other) noexcept
            : impl_(other.impl_)
        {
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*>)
        Ref(const Shared<U>& other) noexcept;

    public:
        [[nodiscard]] Shared<T> lock() const;
        [[nodiscard]] bool      expired() const noexcept { return impl_.expired(); }
        [[nodiscard]]           operator bool() const noexcept { return !expired(); }
        void                    reset() noexcept { impl_.reset(); }
        Ref&                    swap(Ref& other) noexcept
        {
            std::swap(impl_, other.impl_);
            return *this;
        }

    private:
        std::weak_ptr<T> impl_;
    };
} // namespace codex
