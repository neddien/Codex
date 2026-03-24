#pragma once

#include <engine/core/public/exception.h>

#include "sharable.h"

namespace codex::mem {
    // Forward declarations.
    template <typename T>
    class Shared;

    CX_CUSTOM_EXCEPTION(ExpiredRefException, "Weak reference pointer is expired.")

    template <typename T>
    class Ref : public Sharable<T>
    {
        friend class Shared<T>;

    public:
        constexpr Ref() noexcept = default;
        constexpr Ref(std::nullptr_t) noexcept
            : Ref()
        {
        }

        Ref(const Shared<T>& other) noexcept { this->weakly_construct_from(other); }
        Ref(const Ref<T>& other) noexcept { this->weakly_construct_from(other); }
        Ref(Ref<T>&& other) noexcept { this->move_construct_from(std::move(other)); }
        template <typename U>
            requires(std::is_convertible_v<U*, T*> || std::is_base_of_v<T, U>)
        Ref(const Shared<U>& other)
        {
            this->weakly_construct_from(other);
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*> || std::is_base_of_v<T, U>)
        Ref(const Ref<U>& other) noexcept
        {
            this->weakly_construct_from(other);
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*> || std::is_base_of_v<T, U>)
        Ref(Ref<U>&& other) noexcept
        {
            this->move_construct_from(std::move(other));
        }
        ~Ref() noexcept { this->dec_wref(); }

    public:
        [[nodiscard]] operator bool() const noexcept { return !expired(); }
        Ref<T>&       operator=(const Ref<T>& other)
        {
            Ref<T>{ other }.swap(*this);
            return *this;
        }
        Ref<T>& operator=(const Shared<T>& other)
        {
            Ref<T>{ other }.swap(*this);
            return *this;
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*> || std::is_base_of_v<T, U>)
        Ref<T>& operator=(const Ref<U>& other)
        {
            return this->operator=((const Ref<T>&)other);
        }
        template <typename U>
            requires(std::is_convertible_v<U*, T*> || std::is_base_of_v<T, U>)
        Ref<T>& operator=(const Shared<U>& other)
        {
            return this->operator=((const Shared<T>&)other);
        }

    public:
        [[nodiscard]] inline Shared<T> lock() const
        {
            Shared<T> ptr;
            if (!ptr.construct_from_ref(*this))
                throw ExpiredRefException("Cannot lock from an expired weak reference pointer.");
            return ptr;
        }
        inline void               reset() noexcept { Ref<T>{}.swap(*this); }
        [[nodiscard]] inline bool expired() const noexcept { return !this->ctrl_ || this->ctrl_->uses() == 0; }
    };
} // namespace codex::mem
