#ifndef CODEX_MEMORY_BOX_H
#define CODEX_MEMORY_BOX_H

#include <sdafx.h>

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

    private:
        Pointer m_Ptr = nullptr;

    public:
        Box() = default;
        Box(Pointer&& rawPtr) noexcept
            : m_Ptr(rawPtr)
        {
            rawPtr = nullptr;
        }
        Box(Box<T>&& other) noexcept
        {
            m_Ptr       = other.m_Ptr;
            other.m_Ptr = nullptr;
        }
        ~Box() { Drop(); }

    public:
        template <typename U>
            requires(std::is_convertible_v<U, T> || std::is_base_of_v<T, U>)
        Box(Box<U>&& other)
        {
            m_Ptr       = other.m_Ptr;
            other.m_Ptr = nullptr;
        }

    private:
        inline void Drop()
        {
            if (m_Ptr)
            {
                if (std::is_array_v<T>)
                    delete[] m_Ptr;
                else
                    delete m_Ptr;
                m_Ptr = nullptr;
            }
        }

    public:
        [[nodiscard]] constexpr Pointer      Get() noexcept { return m_Ptr; }
        [[nodiscard]] constexpr ConstPointer Get() const noexcept { return m_Ptr; }

    public:
        [[nodiscard]] constexpr                operator bool() const noexcept { return m_Ptr; }
        [[nodiscard]] constexpr Pointer        operator->() noexcept { return m_Ptr; }
        [[nodiscard]] constexpr ConstPointer   operator->() const noexcept { return m_Ptr; }
        [[nodiscard]] constexpr Reference      operator*() noexcept { return *m_Ptr; }
        [[nodiscard]] constexpr ConstReference operator*() const noexcept { return *m_Ptr; }
        inline Box<T>&                         operator=(Box<T>&& other) noexcept
        {
            Box<T>{ std::move(other) }.Swap(*this);
            return *this;
        }
        inline Box<T>& operator=(Pointer&& ptr) noexcept
        {
            if (ptr != m_Ptr)
                Drop();

            m_Ptr = ptr;
            ptr   = nullptr;
            return *this;
        }

    public:
        inline Box<T>& Reset(Pointer&& ptr = nullptr)
        {
            if (ptr == m_Ptr)
                return *this;

            Drop();

            m_Ptr = ptr;
            ptr   = nullptr;
            return *this;
        }
        inline Box<T>& Swap(Box<T>& other) noexcept
        {
            std::swap(m_Ptr, other.m_Ptr);
            return *this;
        }

    public:
        template <typename U>
        [[nodiscard]] Box<U> As() const noexcept
        {
            auto cast_ptr = Box<U>{ static_cast<typename Box<U>::Pointer>(m_Ptr) };
            m_Ptr         = nullptr;
            return std::move(cast_ptr);
        }

    public:
        template <typename... TArgs>
        static inline Box<T> New(TArgs&&... args)
        {
            return Box<T>(new T{ std::forward<TArgs>(args)... });
        }
        static inline Box<T> From(Pointer&& rawPtr) noexcept
        {
            Box<T> obj = std::move(rawPtr);
            return obj;
        }
    };
} // namespace codex::mem

#endif // CODEX_MEMORY_BOX_H
