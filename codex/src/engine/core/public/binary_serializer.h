#pragma once

#include <engine/core/public/serializer.h>
#include <engine/memory/public/memory.h>

namespace codex {
    class BinarySerializationNode : public ISerializationNode
    {
    public:
        BinarySerializationNode() noexcept;
        BinarySerializationNode(std::span<u8> buffer) noexcept;

    public:
        void write(const std::string_view key, const u8 value) override;
        void write(const std::string_view key, const i8 value) override;
        void write(const std::string_view key, const u16 value) override;
        void write(const std::string_view key, const i16 value) override;
        void write(const std::string_view key, const u32 value) override;
        void write(const std::string_view key, const i32 value) override;
        void write(const std::string_view key, const u64 value) override;
        void write(const std::string_view key, const i64 value) override;
        void write(const std::string_view key, const f32 value) override;
        void write(const std::string_view key, const bool value) override;
        void write(const std::string_view key, const Vector2 value) override;
        void write(const std::string_view key, const Vector2f value) override;
        void write(const std::string_view key, const Vector3 value) override;
        void write(const std::string_view key, const Vector3f value) override;
        void write(const std::string_view key, const Vector4f value) override;
        void write(const std::string_view key, const Vector4 value) override;
        void write(const std::string_view key, const Rectf value) override;
        void write(const std::string_view key, const std::string_view value) override;

    public:
        bool read(const std::string_view key, u8& value) const override;
        bool read(const std::string_view key, i8& value) const override;
        bool read(const std::string_view key, u16& value) const override;
        bool read(const std::string_view key, i16& value) const override;
        bool read(const std::string_view key, u32& value) const override;
        bool read(const std::string_view key, i32& value) const override;
        bool read(const std::string_view key, u64& value) const override;
        bool read(const std::string_view key, i64& value) const override;
        bool read(const std::string_view key, f32& value) const override;
        bool read(const std::string_view key, bool& value) const override;
        bool read(const std::string_view key, Vector2& value) const override;
        bool read(const std::string_view key, Vector2f& value) const override;
        bool read(const std::string_view key, Vector3& value) const override;
        bool read(const std::string_view key, Vector3f& value) const override;
        bool read(const std::string_view key, Vector4& value) const override;
        bool read(const std::string_view key, Vector4f& value) const override;
        bool read(const std::string_view key, Rectf& value) const override;
        bool read(const std::string_view key, std::string& value) const override;

    public:
        [[nodiscard]] bool        has_key(const std::string_view key) const noexcept override;
        ISerializationNode&       create_child(const std::string_view key) override;
        const ISerializationNode& child(const std::string_view key) const override;
        ISerializationNode&       begin_array(const std::string_view key) override;
        void                      end_array() override;
        ISerializationNode&       add_array_element() override;
        ISerializationNode&       array(const std::string_view key) const override;
        usize                     array_size() const override;
        const ISerializationNode& array_element(const usize idx) const override;
        void                      for_each_array_element(ArrayCallback fn) const override;
        ISerializationNode&       begin_map(const std::string_view key) override;
        ISerializationNode&       add_map_entry(const std::string_view key) override;
        void                      end_map() override;
        ISerializationNode&       map(const std::string_view key) const override;
        std::vector<std::string>  map_keys() const override;
        const ISerializationNode& map_entry(const std::string_view key) const override;
        void                      for_each_map_entry(MapCallback callback) const override;

    public:
        std::vector<u8> buffer() noexcept;

    private:
        enum EntryType : u8
        {
            KVPair,
            Array,
            Map,
            Child,
        };
        CX_PACKED(struct KeyEntry {
            usize hash;
            union
            {
                usize                    offset;
                BinarySerializationNode* ptr;
            } data;
            EntryType type;
        });

    private:
        template <typename T>
        usize buffer_write(T value) noexcept
        {
            const usize prev_size = buffer_.size();
            buffer_.resize(prev_size + sizeof(T));
            std::memcpy(buffer_.data() + prev_size, &value, sizeof(T));
            return sizeof(T);
        }
        template <typename T>
        usize buffer_write(T* data, const usize len) noexcept
        {
            const usize prev_size = buffer_.size();
            buffer_.resize(prev_size + len * sizeof(T));
            std::memcpy(buffer_.data() + prev_size, data, len * sizeof(T));
            return len * sizeof(T);
        }
        template <typename T>
        void buffer_read_at(const usize offset, T& value) const noexcept
        {
            std::memcpy(&value, buffer_.data() + offset, sizeof(T));
        }
        template <typename T>
        void buffer_read_at(const usize offset, T* data, const usize len) const noexcept
        {
            std::memcpy(data, buffer_.data() + offset, len * sizeof(T));
        }

    private:
        KeyEntry&       add_or_retrieve_key(const std::string_view key);
        const KeyEntry* entry(const std::string_view key) const noexcept;
        KeyEntry*       entry(const std::string_view key) noexcept;

    private:
        std::vector<KeyEntry>                             entries_;
        std::vector<u8>                                   buffer_;
        usize                                             cursor_ = 0;
        mutable std::vector<Box<BinarySerializationNode>> children_;
        mutable bool                                      array_node_ = false;
    };
} // namespace codex
