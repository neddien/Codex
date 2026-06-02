#include "json_archive.h"

namespace codex {
    JsonArchiveBackend::JsonArchiveBackend(json_type& root, const bool saving)
        : saving_{ saving }
    {
        if (saving_ && !root.is_object())
            root = json_type::object();
        frames_.push_back(Frame{ &root, Kind::Object, 0, {} });
    }

    JsonArchiveBackend::json_type& JsonArchiveBackend::save_slot(const std::string_view key)
    {
        Frame& f = top();
        switch (f.kind) {
            case Kind::Object: return (*f.node)[std::string{ key }];
            case Kind::Array: f.node->push_back(json_type{}); return f.node->back();
            case Kind::Map: {
                json_type& slot = (*f.node)[f.pending_key];
                f.pending_key.clear();
                return slot;
            }
        }
        return *f.node; // unreachable
    }

    JsonArchiveBackend::json_type* JsonArchiveBackend::load_slot(const std::string_view key)
    {
        Frame& f = top();
        switch (f.kind) {
            case Kind::Object: {
                const auto it = f.node->find(std::string{ key });
                return it != f.node->end() ? &*it : nullptr;
            }
            case Kind::Array: {
                if (f.index < f.node->size())
                    return &(*f.node)[f.index++];
                return nullptr;
            }
            case Kind::Map: {
                const auto it = f.node->find(f.pending_key);
                f.pending_key.clear();
                return it != f.node->end() ? &*it : nullptr;
            }
        }
        return nullptr;
    }

    template <typename T>
    void JsonArchiveBackend::write(const std::string_view key, const T& value)
    {
        save_slot(key) = value;
    }

    template <typename T>
    void JsonArchiveBackend::readv(const std::string_view key, T& value)
    {
        if (const json_type* slot = load_slot(key); slot && !slot->is_null())
            value = slot->get<T>();
    }

#define CX_JSON_TRIVIAL_IMPL(TYPE)                                                                                     \
    void JsonArchiveBackend::trivial(const std::string_view key, TYPE& value)                                          \
    {                                                                                                                  \
        saving_ ? write<TYPE>(key, value) : readv<TYPE>(key, value);                                                   \
    }

    CX_JSON_TRIVIAL_IMPL(u8)
    CX_JSON_TRIVIAL_IMPL(i8)
    CX_JSON_TRIVIAL_IMPL(u16)
    CX_JSON_TRIVIAL_IMPL(i16)
    CX_JSON_TRIVIAL_IMPL(u32)
    CX_JSON_TRIVIAL_IMPL(i32)
    CX_JSON_TRIVIAL_IMPL(u64)
    CX_JSON_TRIVIAL_IMPL(i64)
    CX_JSON_TRIVIAL_IMPL(f32)
    CX_JSON_TRIVIAL_IMPL(f64)
    CX_JSON_TRIVIAL_IMPL(bool)
    CX_JSON_TRIVIAL_IMPL(std::string)

#undef CX_JSON_TRIVIAL_IMPL

    void JsonArchiveBackend::begin_object(const std::string_view key)
    {
        if (saving_) {
            json_type& slot = save_slot(key);
            slot            = json_type::object();
            frames_.push_back(Frame{ &slot, Kind::Object, 0, {} });
        } else {
            json_type* slot = load_slot(key);
            frames_.push_back(Frame{ slot ? slot : &missing_, Kind::Object, 0, {} });
        }
    }

    void JsonArchiveBackend::end_object()
    {
        frames_.pop_back();
    }

    void JsonArchiveBackend::begin_array(const std::string_view key, usize& count)
    {
        if (saving_) {
            json_type& slot = save_slot(key);
            slot            = json_type::array();
            frames_.push_back(Frame{ &slot, Kind::Array, 0, {} });
        } else {
            json_type* slot = load_slot(key);
            json_type* node = (slot && slot->is_array()) ? slot : &missing_;
            count           = node->size();
            frames_.push_back(Frame{ node, Kind::Array, 0, {} });
        }
    }

    void JsonArchiveBackend::end_array()
    {
        frames_.pop_back();
    }

    void JsonArchiveBackend::begin_map(const std::string_view key, usize& count)
    {
        if (saving_) {
            json_type& slot = save_slot(key);
            slot            = json_type::object();
            frames_.push_back(Frame{ &slot, Kind::Map, 0, {} });
        } else {
            json_type* slot = load_slot(key);
            json_type* node = (slot && slot->is_object()) ? slot : &missing_;
            count           = node->size();
            frames_.push_back(Frame{ node, Kind::Map, 0, {} });
        }
    }

    void JsonArchiveBackend::map_key(std::string& key)
    {
        Frame& f = top();
        if (saving_) {
            f.pending_key = key;
        } else {
            if (f.index < f.node->size()) {
                const auto it = std::next(f.node->begin(), (ptrdiff)(f.index++));
                f.pending_key = it.key();
            } else {
                f.pending_key.clear();
            }
            key = f.pending_key;
        }
    }

    void JsonArchiveBackend::end_map()
    {
        frames_.pop_back();
    }

    bool JsonArchiveBackend::optional(const std::string_view key, const bool present_on_save)
    {
        if (saving_)
            return present_on_save;

        const Frame& f = top();
        if (f.kind == Kind::Object)
            return f.node->contains(std::string{ key });
        return true;
    }
} // namespace codex
