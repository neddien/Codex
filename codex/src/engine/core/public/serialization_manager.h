#pragma once

#include <engine/core/public/archive.h>

namespace codex {
    // High-level entry points. Save paths take a const object and const_cast at the boundary:
    // the single archive() function is non-const because loading mutates, but save mode
    // provably only reads the object.
    class CODEX_API SerializationManager
    {
    public:
        enum class Format
        {
            Json,
            Binary,
        };

    public:
        [[nodiscard]] static std::string     to_json(const ISerializable& object);
        static void                          from_json(ISerializable& object, const std::string_view json);
        [[nodiscard]] static std::vector<u8> to_binary(const ISerializable& object);
        static void                          from_binary(ISerializable& object, std::span<const u8> data);

        // Format chosen by extension: ".json" → JSON text, otherwise binary.
        static bool save_to_file(const ISerializable& object, const std::filesystem::path& filepath,
                                 const Format format = Format::Json);
        static bool load_from_file(ISerializable& object, const std::filesystem::path& filepath, const Format format);
    };
} // namespace codex
