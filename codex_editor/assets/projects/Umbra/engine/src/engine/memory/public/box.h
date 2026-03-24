#pragma once

namespace codex::mem {
    template <typename T>
    class Box
    {
        template <typename U>
        friend class Box; // We don't ask questions here.

    private:
        using BaseType       = typename std::remove_extent_t<T>;
        using Pointer        = BaseType*;
        using ConstPointer   = const BaseType*;
        using Reference      = BaseType&;
        using ConstReference = const BaseType&;

    public:
        Box() = default;
        constexpr Box(std::nullptr_t) noexcept
            : Box()
        {
        }
        explicit Box(Pointer&& raw_ptr) noexcept
            : ptr_(raw_ptr)
        {
            raw_ptr = nullptr;
        }
        Box(Box<T>&& other) noexcept
        {
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        ~Box() { drop(); }

    public:
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Box(Box<U>&& other)
        {
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }

    public:
        [[nodiscard]] constexpr Pointer      get() noexcept { return ptr_; }
        [[nodiscard]] constexpr ConstPointer get() const noexcept { return ptr_; }

    public:
        [[nodiscard]] constexpr                operator bool() const noexcept { return ptr_; }
        [[nodiscard]] constexpr Pointer        operator->() noexcept { return ptr_; }
        [[nodiscard]] constexpr ConstPointer   operator->() const noexcept { return ptr_; }
        [[nodiscard]] constexpr Reference      operator*() noexcept { return *ptr_; }
        [[nodiscard]] constexpr ConstReference operator*() const noexcept { return *ptr_; }
        inline Box<T>&                         operator=(Box<T>&& other) noexcept
        {
            Box<T>{ std::move(other) }.swap(*this);
            return *this;
        }
        inline Box<T>& operator=(Pointer&& ptr) noexcept
        {
            if (ptr != ptr_)
                drop();

            ptr_ = ptr;
            ptr  = nullptr;
            return *this;
        }

    public:
        inline Box<T>& reset(Pointer&& ptr = nullptr)
        {
            if (ptr == ptr_)
                return *this;

            drop();

            ptr_ = ptr;
            ptr  = nullptr;
            return *this;
        }
        inline Box<T>& swap(Box<T>& other) noexcept
        {
            std::swap(ptr_, other.ptr_);
            return *this;
        }

    public:
        template <typename U>
        [[nodiscard]] Box<U> as() const noexcept
        {
            auto cast_ptr = Box<U>{ static_cast<typename Box<U>::Pointer>(ptr_) };
            ptr_          = nullptr;
            return std::move(cast_ptr);
        }

    public:
        template <typename... TArgs>
        [[nodiscard]] static inline Box<T> make(TArgs&&... args)
        {
            return Box<T>(new T{ std::forward<TArgs>(args)... });
        }
        [[nodiscard]] static inline Box<T> from(Pointer&& raw_ptr) noexcept
        {
            Box<T> obj{ std::move(raw_ptr) };
            return obj;
        }

    private:
        inline void drop()
        {
            if (ptr_) {
                if (std::is_array_v<T>)
                    delete[] ptr_;
                else
                    delete ptr_;
                ptr_ = nullptr;
            }
        }

    private:
        Pointer ptr_ = nullptr;
    };
} // namespace codex::mem
