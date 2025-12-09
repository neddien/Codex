#ifndef CODEX_CONCURRENCY_MUTEX_H
#define CODEX_CONCURRENCY_MUTEX_H

#include <sdafx.h>

#include <Engine/Core/Public/CommonDef.h>

namespace codex::cc {
    // TODO: Rename to Sync and create Codex Exceptions for CC stuff and handle them here.
    // TODO: Like Sync in mgrd, add debug checking for the same thread trying to lock twice.
    template <typename T>
    class Mutex
    {
    public:
        class ScopedGuard;

    private:
        T                         m_Object;
        mutable std::mutex        m_Mutex;
        mutable std::atomic<bool> m_Locked;
        mutable std::thread::id   m_OwnerThread;

    public:
        template <typename... TArgs>
        constexpr Mutex(TArgs&&... args)
            : m_Object(std::forward<TArgs>(args)...)
            , m_Locked(false)
        {
        }
        inline Mutex(const Mutex<T>& other)               = delete;
        inline Mutex<T>& operator=(const Mutex<T>& other) = delete;
        inline Mutex(Mutex<T>&& other) noexcept
        {
            if (this != &other)
            {
                const auto lock = other.Lock();

                m_Object = std::move(other.m_Object);
            }
        }
        constexpr Mutex<T>& operator=(Mutex<T>&& other) noexcept
        {
            Mutex<T>{ std::move(other) }.Swap(*this);
            return *this;
        }
        constexpr ~Mutex() noexcept
        {
            if (IsLockedByCurrentThread())
            {
                MutexUnlock();
            }
        }

    public:
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
        inline void MutexLock() const noexcept
        {
            m_Mutex.lock();
            m_OwnerThread = std::this_thread::get_id();
            m_Locked.store(true, std::memory_order_release);
        }
        [[nodiscard]] inline bool MutexTryLock() const noexcept
        {
            if (m_Mutex.try_lock())
            {
                m_OwnerThread = std::this_thread::get_id();
                m_Locked.store(true, std::memory_order_release);
                return true;
            }
            return false;
        }
        inline void MutexUnlock() const noexcept
        {
            if (m_Locked.load(std::memory_order_acquire) && m_OwnerThread == std::this_thread::get_id())
            {
                m_Locked.store(false, std::memory_order_release);
                m_OwnerThread = std::thread::id{};
                m_Mutex.unlock();
            }
        }

    public:
        [[nodiscard]] inline bool IsLocked() const noexcept { return m_Locked.load(std::memory_order_acquire); }
        [[nodiscard]] inline bool IsLockedByCurrentThread() const noexcept
        {
            return IsLocked() && m_OwnerThread == std::this_thread::get_id();
        }
        [[nodiscard]] inline ScopedGuard       Lock() noexcept { return ScopedGuard(*this); }
        [[nodiscard]] inline const ScopedGuard Lock() const { return const_cast<Mutex<T>*>(this)->Lock(); }
        inline void                            Swap(Mutex<T>& other) noexcept
        {
            const auto lock  = Lock();
            const auto lock1 = other.Lock();

            std::swap(m_Object, other.m_Object);
        }

    public:
        template <typename... TArgs>
        [[nodiscard]] static inline Mutex New(TArgs&&... args) noexcept
        {
            return Mutex<T>(std::forward<TArgs>(args)...);
        }
    };

    template <typename T>
    class Mutex<T>::ScopedGuard
    {
        friend class Mutex<T>;

    private:
        Mutex<T>& m_Mutex;

    private:
        constexpr ScopedGuard(Mutex<T>& mutex)
            : m_Mutex(mutex)
        {
            m_Mutex.MutexLock();
        }

    public:
        constexpr ~ScopedGuard() noexcept { m_Mutex.MutexUnlock(); }

    public:
        ScopedGuard(const ScopedGuard&) noexcept            = delete;
        ScopedGuard& operator=(const ScopedGuard&) noexcept = delete;
        ScopedGuard(ScopedGuard&&) noexcept                 = delete;
        ScopedGuard& operator=(ScopedGuard&&) noexcept      = delete;

    public:
        [[nodiscard]] constexpr T*       operator->() noexcept { return std::addressof(m_Mutex.m_Object); }
        [[nodiscard]] constexpr const T* operator->() const noexcept
        {
            return const_cast<ScopedGuard*>(this)->operator->();
        }
        [[nodiscard]] constexpr T&       operator*() noexcept { return m_Mutex.m_Object; }
        [[nodiscard]] constexpr const T& operator*() const noexcept
        {
            return const_cast<ScopedGuard*>(this)->operator*();
        }
    };
} // namespace codex::cc

#endif // CODEX_CONCURRENCY_MUTEX_H
