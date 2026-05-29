#include "public/binary_serializer.h"

#define BINSER_TRIVIAL_RW_IMPL_FOR(TYPE)                                                                               \
    void BinarySerializationNode::write(const std::string_view key, const TYPE value)                                  \
    {                                                                                                                  \
        KeyEntry& entry   = add_or_retrieve_key(key);                                                                  \
        entry.data.offset = cursor_;                                                                                   \
        entry.type        = EntryType::KVPair;                                                                         \
        cursor_ += buffer_write<TYPE>(value);                                                                          \
    }                                                                                                                  \
    bool BinarySerializationNode::read(const std::string_view key, TYPE& value) const                                  \
    {                                                                                                                  \
        const KeyEntry* key_entry = entry(key);                                                                        \
        if (!key_entry || key_entry->type != EntryType::KVPair)                                                        \
            return false;                                                                                              \
                                                                                                                       \
        buffer_read_at<TYPE>(key_entry->data.offset, value);                                                           \
                                                                                                                       \
        return true;                                                                                                   \
    }

namespace codex {
    BinarySerializationNode::BinarySerializationNode() noexcept = default;
    BinarySerializationNode::BinarySerializationNode(std::span<u8> buffer) noexcept
    {
        const u64 len = *reinterpret_cast<u64*>(buffer.data());
        entries_.resize(len);

        std::memcpy(entries_.data(), buffer_.data() + sizeof(u64), len * sizeof(KeyEntry));

        std::span<u8> data_buffer = buffer.subspan(sizeof(u64) + len * sizeof(KeyEntry));
        buffer_                   = std::vector<u8>{ data_buffer.begin(), data_buffer.end() };
    }

    BINSER_TRIVIAL_RW_IMPL_FOR(u8)
    BINSER_TRIVIAL_RW_IMPL_FOR(i8)
    BINSER_TRIVIAL_RW_IMPL_FOR(u16)
    BINSER_TRIVIAL_RW_IMPL_FOR(i16)
    BINSER_TRIVIAL_RW_IMPL_FOR(u32)
    BINSER_TRIVIAL_RW_IMPL_FOR(i32)
    BINSER_TRIVIAL_RW_IMPL_FOR(u64)
    BINSER_TRIVIAL_RW_IMPL_FOR(i64)
    BINSER_TRIVIAL_RW_IMPL_FOR(f32)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector2)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector2f)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector3)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector3f)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector4)
    BINSER_TRIVIAL_RW_IMPL_FOR(Vector4f)
    BINSER_TRIVIAL_RW_IMPL_FOR(Rectf)

    void BinarySerializationNode::write(const std::string_view key, const std::string_view value)
    {
        KeyEntry& entry   = add_or_retrieve_key(key);
        entry.data.offset = cursor_;
        entry.type        = EntryType::KVPair;
        cursor_ += buffer_write<u64>(value.size());
        cursor_ += buffer_write<u8>((u8*)value.data(), value.size());
    }

    bool BinarySerializationNode::read(const std::string_view key, std::string& value) const
    {
        const KeyEntry* key_entry = entry(key);
        if (!key_entry || key_entry->type != EntryType::KVPair)
            return false;

        u64 len;
        buffer_read_at<u64>(key_entry->data.offset, len);
        buffer_read_at<u8>(key_entry->data.offset + sizeof(u64), (u8*)value.data(), len);

        return true;
    }

    bool BinarySerializationNode::has_key(const std::string_view key) const noexcept
    {
        return std::find_if(entries_.begin(), entries_.end(), [hash = util::crypto::fnv1a(key)](const KeyEntry& entry)
                            { return entry.hash == hash; }) != entries_.end();
    }

    ISerializationNode& BinarySerializationNode::create_child(const std::string_view key)
    {
        KeyEntry& key_entry = add_or_retrieve_key(key);
        key_entry.type      = EntryType::Child;

        children_.push_back(Box<BinarySerializationNode>::make());

        key_entry.data.ptr = children_.back().get();

        return *children_.back();
    }

    ISerializationNode& BinarySerializationNode::begin_array(const std::string_view key)
    {
        KeyEntry& key_entry = add_or_retrieve_key(key);
        key_entry.type      = EntryType::Array;

        auto child         = Box<BinarySerializationNode>::make();
        child->array_node_ = true;
        key_entry.data.ptr = child.get();

        children_.push_back(Box<BinarySerializationNode>::make());
        return *children_.back();
    }

    void BinarySerializationNode::end_array()
    {
    }

    ISerializationNode& BinarySerializationNode::add_array_element()
    {
        assert(array_node_ && "add_array_element called on non-array node");
    }

    std::vector<u8> BinarySerializationNode::buffer() noexcept
    {
        std::vector<u8> serbin(sizeof(u64) + entries_.size() * sizeof(KeyEntry) + buffer_.size());
        *reinterpret_cast<u64*>(serbin.data()) = entries_.size();
        std::memcpy(serbin.data() + sizeof(u64), entries_.data(), entries_.size() * sizeof(KeyEntry));
        serbin.insert(serbin.end(), buffer_.begin(), buffer_.end());
        return serbin;
    }

    BinarySerializationNode::KeyEntry& BinarySerializationNode::add_or_retrieve_key(const std::string_view key)
    {
        const usize hash = util::crypto::fnv1a(key);
        auto        it   = std::find_if(entries_.begin(), entries_.end(),
                                        [hash = util::crypto::fnv1a(key)](const KeyEntry& entry) { return entry.hash == hash; });
        if (it != entries_.end())
            return *it;

        KeyEntry new_entry{
            .hash        = hash,
            .data.offset = cursor_,
        };

        entries_.push_back(std::move(new_entry));

        return entries_.back();
    }

    const BinarySerializationNode::KeyEntry* BinarySerializationNode::entry(const std::string_view key) const noexcept
    {
        auto it = std::find_if(entries_.begin(), entries_.end(),
                               [hash = util::crypto::fnv1a(key)](const KeyEntry& entry) { return entry.hash == hash; });
        if (it != entries_.end())
            return std::addressof(*it);

        return nullptr;
    }

    BinarySerializationNode::KeyEntry* BinarySerializationNode::entry(const std::string_view key) noexcept
    {
        return const_cast<KeyEntry*>(std::as_const(*this).entry(key));
    }
} // namespace codex
