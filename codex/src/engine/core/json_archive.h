#pragma once

#include <engine/core/public/archive.h>

#include <nlohmann/json.hpp>

namespace codex {
    // IArchiveBackend over nlohmann::ordered_json. One instance handles a single direction
    // (save or load), chosen at construction. Save builds the tree; load walks an existing
    // one. A frame stack tracks the current container so primitives know whether to key
    // (object), append (array), or use the pending map key (map).
    class JsonArchiveBackend : public IArchiveBackend
    {
        using json_type = nlohmann::ordered_json;

    public:
        // Save: starts an object tree rooted at `root` (expected to be writable/empty).
        // Load: reads from the already-parsed `root`.
        JsonArchiveBackend(json_type& root, const bool saving);

    public:
        [[nodiscard]] bool saving() const override { return saving_; }

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
        void               begin_object(const std::string_view key) override;
        void               end_object() override;
        void               begin_array(const std::string_view key, usize& count) override;
        void               end_array() override;
        void               begin_map(const std::string_view key, usize& count) override;
        void               map_key(std::string& key) override;
        void               end_map() override;
        [[nodiscard]] bool optional(const std::string_view key, const bool present_on_save) override;

    private:
        enum class Kind
        {
            Object,
            Array,
            Map,
        };
        struct Frame
        {
            json_type*  node;
            Kind        kind;
            usize       index = 0; // array element / map key cursor
            std::string pending_key;
        };

    private:
        [[nodiscard]] Frame& top() noexcept { return frames_.back(); }

        // Save: place `value` at the right slot for the current frame.
        json_type& save_slot(const std::string_view key);
        // Load: locate the json for `key`/position in the current frame, or nullptr if absent.
        json_type* load_slot(const std::string_view key);

        template <typename T>
        void write(const std::string_view key, const T& value);
        template <typename T>
        void readv(const std::string_view key, T& value);

    private:
        bool               saving_;
        std::vector<Frame> frames_;
        json_type          missing_; // null sentinel pointed at by frames for absent slots on load
    };
} // namespace codex
