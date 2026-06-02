#pragma once

#include <engine/core/public/archive.h>

namespace codex {
    // IArchiveBackend as a flat, ordered byte stream. Because save and load traverse the
    // exact same field order, no keys, hashes, or offset tables are needed: save appends,
    // load consumes from a cursor. Structural scopes (objects) are pure no-ops; only the
    // element/entry counts and map keys carry structural bytes.
    class CODEX_API BinaryArchiveBackend : public IArchiveBackend
    {
    public:
        BinaryArchiveBackend() noexcept;                                  // save mode
        explicit BinaryArchiveBackend(std::span<const u8> data) noexcept; // load mode

    public:
        [[nodiscard]] bool saving() const override { return saving_; }

        // Hand off the accumulated bytes (save mode only).
        [[nodiscard]] std::vector<u8> take_buffer() noexcept { return std::move(out_); }

    public:
        void trivial(const std::string_view key, u8& value) override;
        void trivial(const std::string_view key, i8& value) override;
        void trivial(const std::string_view key, u16& value) override;
        void trivial(const std::string_view key, i16& value) override;
        void trivial(const std::string_view key, u32& value) override;
        void trivial(const std::string_view key, i32& value) override;
        void trivial(const std::string_view key, u64& value) override;
        void trivial(const std::string_view key, i64& value) override;
        void trivial(const std::string_view key, f32& value) override;
        void trivial(const std::string_view key, f64& value) override;
        void trivial(const std::string_view key, bool& value) override;
        void trivial(const std::string_view key, std::string& value) override;

    public:
        void               begin_object(const std::string_view key) override {}
        void               end_object() override {}
        void               begin_array(const std::string_view key, usize& count) override;
        void               end_array() override {}
        void               begin_map(const std::string_view key, usize& count) override;
        void               map_key(std::string& key) override;
        void               end_map() override {}
        [[nodiscard]] bool optional(const std::string_view key, const bool present_on_save) override;

    private:
        void ensure(const usize bytes) const;
        template <typename T>
        void rw_trivial(T& value);
        void rw_string(std::string& value);
        void rw_count(usize& count);

    private:
        bool            saving_;
        std::vector<u8> out_;               // save target
        const u8*       in_      = nullptr; // load source
        usize           in_size_ = 0;
        usize           cursor_  = 0;
    };
} // namespace codex
