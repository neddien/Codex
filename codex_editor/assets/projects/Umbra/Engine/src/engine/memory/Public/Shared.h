#pragma once

#include <engine/core/public/exception.h>

#include "sharable.h"

namespace codex::mem {
    // Forward declarations.
    template <typename T>
    class Ref;
    template <typename T>
    class Shared;

    template <typename T>
    class SharedManagable
    {
        template <typename U>
        friend class Shared;

    public:
        using E_Type = SharedManagable;

    protected:
        Shared<T> new_shared_from_this() noexcept { return weak_ref_.lock(); }
        Ref<T>    new_ref_from_this() noexcept { return weak_ref_; }

    protected:
        Ref<T> weak_ref_ = nullptr;
    };

    template <class T, class = void>
    struct CanSharedManagable : std::false_type
    {
    }; // detect unambiguous and accessible inheritance from enable_shared_from_this

    template <class T>
    struct CanSharedManagable<T, std::void_t<typename T::E_Type>>
        : std::is_convertible<std::remove_cv_t<T>*, typename T::E_Type*>::type
    {
        // is_convertible is necessary to verify unambiguous inheritance
    };

    template <typename T>
    class Shared : public Sharable<T>
    {
        template <typename U>
        friend class Shared; // We don't ask questions here.

        friend class Ref<T>;

    public:
        using typename Sharable<T>::BaseType;
        using typename Sharable<T>::Pointer;
        using typename Sharable<T>::ConstPointer;
        using typename Sharable<T>::Reference;
        using typename Sharable<T>::ConstReference;
        using typename Sharable<T>::DifferenceType;

    public:
        constexpr Shared() = default;
        constexpr Shared(std::nullptr_t)
            : Shared()
        {
        }

        explicit Shared(const Pointer ptr)
        {
            if constexpr (std::is_array_v<T>)
                init_manager(ptr, new ManagedResource(ptr, std::default_delete<T[]>{}));
            else
                init_manager(ptr, new ManagedMemory(ptr));
        }
        template <typename Deleter>
            requires(std::is_invocable_v<Deleter>)
        Shared(const Pointer ptr, Deleter deleter)
        {
            init_manager(ptr, new ManagedResource(ptr, std::move(deleter)));
        }
        Shared(const Shared<T>& other) { this->copy_construct_from(other); }
        Shared(Shared<T>&& other) noexcept { this->move_construct_from(std::move(other)); }
        ~Shared() noexcept { this->dec_ref(); }

    public:
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        explicit Shared(U* const ptr)
            : Shared((const Pointer)ptr)
        {
        }
        template <typename U, typename Deleter>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared(U* const ptr, Deleter deleter)
        {
            init_manager(ptr, new ManagedResource(ptr, std::move(deleter)));
        }
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared(const Shared<U>& other)
        {
            this->copy_construct_from(other);
        }
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared(Shared<U>&& other) noexcept
        {
            this->move_construct_from(std::move(other));
        }

    private:
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared(U* const ptr, ManagableObject* const ctrl) noexcept
        {
            init_manager(ptr, ctrl);
        }

    public:
        Shared<T>& operator=(const Shared<T>& other)
        {
            Shared<T>{ other }.swap(*this);
            return *this;
        }
        Shared<T>& operator=(Shared<T>&& other) noexcept
        {
            Shared<T>{ std::move(other) }.swap(*this);
            return *this;
        }
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared<T>& operator=(const Shared<U>& other)
        {
            return this->operator=((const Shared<T>&)other);
        }
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Shared<T>& operator=(Shared<U>&& other) noexcept
        {
            return this->operator=(std::move((Shared<T>&)other));
        }

    public:
        [[nodiscard]] inline Ref<T>       as_ref() noexcept { return *this; }
        [[nodiscard]] inline Ref<const T> as_ref() const noexcept { return *this; }

    public:
        constexpr                operator bool() const noexcept { return this->get() != nullptr; }
        constexpr Pointer        operator->() noexcept { return this->get(); }
        constexpr ConstPointer   operator->() const noexcept { return this->get(); }
        constexpr Reference      operator*() noexcept { return *(this->get()); }
        constexpr ConstReference operator*() const noexcept { return *(this->get()); }

    public:
        inline Shared<T>& reset(Pointer&& ptr = nullptr)
        {
            Shared<T>{ std::move(ptr) }.swap(*this);
            return *this;
        }

    public:
        template <typename U>
        Shared<U> as() const
        {
            auto cast_ptr = Shared<U>{ static_cast<Shared<U>::Pointer>(this->ptr_), this->ctrl_ };
            this->inc_ref();
            return cast_ptr;
        }

    public:
        template <typename... TArgs>
        [[nodiscard]] static inline Shared<T> make(TArgs&&... args)
        {
            return std::move(Shared<T>{ new T(std::forward<TArgs>(args)...) });
        }
        [[nodiscard]] static inline Shared<T> from(Pointer&& raw_ptr)
        {
            Shared<T> obj{ std::move(raw_ptr) };
            return obj;
        }

    private:
        template <typename U>
        void init_manager(U* const ptr, ManagableObject* const ctrl) noexcept
        {
            this->ptr_  = ptr;
            this->ctrl_ = ctrl;

            // This means that T is (directly or indirectly) inheriting from SharedManagable
            //  thus should be able to create a copy (not a new) Shared<T> from within itself.
            // This means we need to assign the weak_ref_ to point to this.
            if constexpr (CanSharedManagable<T>::value) {
                if (this->ptr_ && this->ptr_->weak_ref_.expired())
                    this->ptr_->weak_ref_ = *this;
            }
        }
    };
} // namespace codex::mem
