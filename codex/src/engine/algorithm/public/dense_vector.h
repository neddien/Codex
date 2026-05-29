#pragma once

#include "u1648id.h"

#include <engine/core/public/exception.h>

namespace codex {
    CX_CUSTOM_EXCEPTION(WeakReferenceExpired, "Weak reference has been expired");

    template <typename T>
    class dense_vector
    {
    public:
        using value_type      = T;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = T*;
        using const_pointer   = const T*;
        using size_type       = usize;
        using id_type         = u1648id;
        using vec_type        = std::vector<value_type>;
        using iterator        = vec_type::iterator;
        using const_iterator  = vec_type::const_iterator;

    public:
        dense_vector() noexcept = default;
        explicit dense_vector(const size_type len)
        {
            dense_.reserve(len);
            for (size_type i = 0; i < len; ++i) {
                push_back({});
            }
        }
        explicit(false) dense_vector(std::initializer_list<T> list) noexcept
        {
            dense_.reserve(list.size());
            for (const auto& e : list) {
                push_back(e);
            }
        }
        dense_vector(const dense_vector& other)                = default;
        dense_vector(dense_vector&& other) noexcept            = default;
        dense_vector& operator=(const dense_vector& other)     = default;
        dense_vector& operator=(dense_vector&& other) noexcept = default;

    public:
        [[nodiscard]] size_type size() const noexcept { return dense_.size(); }
        [[nodiscard]] bool      empty() const noexcept { return dense_.empty(); }
        [[nodiscard]] size_type capacity() const noexcept { return dense_.capacity(); }
        void                    clear() noexcept
        {
            dense_.clear();
            dense_to_slot_.clear();
            free_slots_.clear();
            for (usize i = 0; i < slots_.size(); ++i) {
                slots_[i].inc_gen();
                free_slots_.push_back(i);
            }
        }
        iterator       begin() noexcept { return dense_.begin(); }
        const_iterator begin() const noexcept { return dense_.begin(); }
        iterator       end() noexcept { return dense_.end(); }
        const_iterator end() const noexcept { return dense_.end(); }

    public:
        id_type push_back(const T& data)
        {
            // clang-format: no inline
            return emplace_back(data);
        }
        template <typename... TArgs>
        id_type emplace_back(TArgs&&... args)
        {
            id_type id{ id_type::invalid_id() };

            dense_.emplace_back(std::forward<TArgs>(args)...);

            if (free_slots_.empty()) {
                // Generation will always be 0 because this is a new slot
                id = id_type{ 0, dense_.size() - 1 };
                slots_.emplace_back(id);
                dense_to_slot_.emplace_back(id.index());
            } else {
                u64 current_slot_idx = free_slots_.back();
                free_slots_.pop_back();

                slots_[current_slot_idx].set_index(dense_.size() - 1);
                id = id_type{ slots_[current_slot_idx].gen(), current_slot_idx };

                dense_to_slot_.emplace_back(current_slot_idx);
            }

            return id;
        }
        void pop_back()
        {
            assert(!empty());
            const u64 slot_idx = dense_to_slot_.back();
            erase(id_type{ slots_[slot_idx].gen(), slot_idx });
        }
        void erase(id_type id)
        {
            if (id.index() >= slots_.size())
                throw IndexOutOfBoundsException("Element does not exist");

            id_type& slot_entry = slots_[id.index()];

            if (slot_entry.gen() != id.gen())
                throw WeakReferenceExpired();

            u64 last_index = dense_.size() - 1;

            std::swap(dense_[slot_entry.index()], dense_.back());
            dense_.pop_back();

            slots_[dense_to_slot_[last_index]].set_index(slot_entry.index());
            dense_to_slot_[slot_entry.index()] = dense_to_slot_[last_index];
            dense_to_slot_.pop_back();

            // Invalidate this slot for weak references by bumping up the generation
            slot_entry.inc_gen();

            // Append this to the free id list for reuse
            free_slots_.push_back(id.index());
        }
        [[nodiscard]] reference at(id_type id)
        {
            // clang-format: no inline
            return const_cast<reference>(std::as_const(*this).at(id));
        }
        [[nodiscard]] const_reference at(id_type id) const
        {
            // clang-format: no inline
            if (id.index() >= slots_.size())
                throw IndexOutOfBoundsException("Element does not exist");

            const id_type slot_entry = slots_[id.index()];
            if (slot_entry.gen() != id.gen())
                throw WeakReferenceExpired();
            else if (slot_entry.index() >= dense_.size())
                throw IndexOutOfBoundsException("Element does not exist");

            return dense_[slot_entry.index()];
        }
        [[nodiscard]] pointer try_at(id_type id) noexcept
        {
            // clang-format: no inline
            return const_cast<pointer>(std::as_const(*this).try_at(id));
        }
        [[nodiscard]] const_pointer try_at(id_type id) const noexcept
        {
            // clang-format: no inline
            if (id.index() < slots_.size()) {
                const id_type slot_entry = slots_[id.index()];
                if (slot_entry.gen() == id.gen() && slot_entry.index() < dense_.size())
                    return &dense_[slot_entry.index()];
            }
            return nullptr;
        }
        [[nodiscard]] reference operator[](id_type id) noexcept
        {
            // clang-format: no inline
            return const_cast<reference>(std::as_const(*this).operator[](id));
        }
        [[nodiscard]] const_reference operator[](id_type id) const noexcept
        {
            assert(id.index() < slots_.size());
            const id_type slot_entry = slots_[id.index()];
            assert(slot_entry.gen() == id.gen());
            assert(slot_entry.index() < dense_.size());
            return dense_[slot_entry.index()];
        }
        [[nodiscard]] bool contains(id_type id) const noexcept
        {
            if (id.index() >= slots_.size())
                return false;
            const id_type slot_entry = slots_[id.index()];
            return slot_entry.gen() == id.gen() && slot_entry.index() < dense_.size();
        }

    public:
        dense_vector& swap(dense_vector& other) noexcept
        {
            std::swap(dense_, other.dense_);
            std::swap(slots_, other.slots_);
            std::swap(free_slots_, other.free_slots_);
            std::swap(dense_to_slot_, other.dense_to_slot_);
            return *this;
        }
        std::string to_string() const noexcept
        {
            std::string str;
            str += "[";
            for (const auto& e : dense_) {
                str += " " + e + ',';
            }
            str.pop_back();
            str += " ]";

            str += " : [";
            for (const auto& e : slots_) {
                str += " (" + std::to_string(e.gen()) + ", " + std::to_string(e.index()) + "),";
            }
            str.pop_back();
            str += " ]";
            return str;
        }

    private:
        vec_type             dense_;
        std::vector<id_type> slots_;
        std::vector<u64>     free_slots_;
        std::vector<u64>     dense_to_slot_;
    };
} // namespace codex
