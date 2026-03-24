#pragma once

namespace codex::mem {
    // Forward declaration.
    template <typename T>
    class Shared;
    template <typename T>
    class Ref;

    // Base abstract resource manager class using reference counting.
    class ManagableObject
    {
    protected:
        constexpr ManagableObject()         = default;
        virtual ~ManagableObject() noexcept = default;

    protected:
        ManagableObject(const ManagableObject&)            = delete;
        ManagableObject& operator=(const ManagableObject&) = delete;

    protected:
        virtual void drop() noexcept      = 0;
        virtual void drop_self() noexcept = 0;

    public:
        [[nodiscard]] usize uses() const noexcept { return uses_.load(); }
        [[nodiscard]] usize weaks() const noexcept { return weaks_.load(); }

    public:
        void inc_ref() noexcept { ++uses_; }
        bool inc_ref_nz() noexcept
        {
            if (uses_ != 0) {
                ++uses_;
                return true;
            }
            return false;
        }
        void inc_wref() noexcept { ++weaks_; }
        void dec_ref() noexcept
        {
            if (--uses_ == 0) {
                drop();
                dec_wref();
            }
        }
        void dec_wref() noexcept
        {
            if (--weaks_ == 0) {
                drop_self();
            }
        }

    protected:
        std::atomic<usize> uses_  = 1;
        std::atomic<usize> weaks_ = 1;
    };

    template <typename T>
    class ManagedMemory : public ManagableObject
    {
    public:
        explicit ManagedMemory(T* const ptr) noexcept
            : ptr_(ptr)
        {
        }
        ~ManagedMemory() noexcept override {}

    private:
        void drop() noexcept override { delete ptr_; }
        void drop_self() noexcept override { delete this; }

    private:
        T* ptr_ = nullptr;
    };

    // Resource manager with a custom deleter.
    template <typename T, typename Deleter>
    class ManagedResource : public ManagableObject
    {
    public:
        ManagedResource(T* const ptr, Deleter deleter) noexcept
            : ptr_(ptr)
            , deleter_(deleter)
        {
        }
        ~ManagedResource() noexcept override { this->drop(); }

    private:
        void drop() noexcept override
        {
            if (ptr_) {
                deleter_(ptr_);
                ptr_   = nullptr;
                uses_  = 1;
                weaks_ = 1;
            }
        }
        void drop_self() noexcept override { delete this; }

    private:
        T*      ptr_ = nullptr;
        Deleter deleter_;
    };

    // TODO: Resource manager with a custom deleter and an allocator.
    // template <typename T, typename Deleter, typename Allocator>
    // class MemoryControllerAlloc : public ManagableObject

    template <typename T>
    class Sharable
    {
        template <typename U>
        friend class Sharable;

        friend class Shared<T>;

        template <typename U>
        friend class Ref;

    public:
        using BaseType       = std::remove_extent_t<T>;
        using Pointer        = BaseType*;
        using ConstPointer   = const BaseType*;
        using Reference      = BaseType&;
        using ConstReference = const BaseType&;
        using DifferenceType = std::ptrdiff_t;

    public:
        constexpr Sharable() noexcept = default;
        ~Sharable()                   = default;

    protected:
        template <typename U>
        void move_construct_from(Sharable<U>&& other) noexcept
        {
            ptr_  = other.ptr_;
            ctrl_ = other.ctrl_;

            other.ptr_  = nullptr;
            other.ctrl_ = nullptr;
        }
        template <typename U>
        void copy_construct_from(const Sharable<U>& other) noexcept
        {
            other.inc_ref();

            ptr_  = other.ptr_;
            ctrl_ = other.ctrl_;
        }
        template <typename U>
        void alias_construct_from(const Sharable<U>& other, Pointer alias_ptr) noexcept
        {
            other.inc_ref();

            ptr_  = alias_ptr;
            ctrl_ = other.ctrl_;
        }
        template <typename U>
        void alias_move_construct_from(Sharable<U>&& other, Pointer alias_ptr) noexcept
        {
            ptr_  = alias_ptr;
            ctrl_ = other.ctrl_;

            other.ptr_  = nullptr;
            other.ctrl_ = nullptr;
        }
        template <typename U>
        bool construct_from_ref(const Ref<U>& other) noexcept
        {
            if (other.ctrl_ && other.ctrl_->inc_ref_nz()) {
                ptr_  = other.ptr_;
                ctrl_ = other.ctrl_;
                return true;
            }
            return false;
        }
        template <typename U>
        void weakly_construct_from(const Sharable<U>& other) noexcept
        {
            if (other.ctrl_) {
                ptr_  = other.ptr_;
                ctrl_ = other.ctrl_;
                ctrl_->inc_wref();
            }
        }

    protected:
        void inc_ref() const noexcept
        {
            if (ctrl_)
                ctrl_->inc_ref();
        }
        void dec_ref() const noexcept
        {
            if (ctrl_)
                ctrl_->dec_ref();
        }
        void inc_wref() const noexcept
        {
            if (ctrl_)
                ctrl_->inc_wref();
        }
        void dec_wref() const noexcept
        {
            if (ctrl_)
                ctrl_->dec_wref();
        }

    public:
        [[nodiscard]] Pointer get() const noexcept { return ptr_; }
        Sharable<T>&          swap(Sharable& other) noexcept
        {
            std::swap(ptr_, other.ptr_);
            std::swap(ctrl_, other.ctrl_);
            return *this;
        }

    protected:
        Pointer          ptr_  = nullptr;
        ManagableObject* ctrl_ = nullptr;
    };
} // namespace codex::mem
