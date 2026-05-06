#pragma once

#include <engine/core/public/serializer.h>

namespace codex {
    class CODEX_API SerializationManager
    {
    public:
        static std::string serialize_to_json(const ISerializable& object);
        static void        deserialize_from_json(ISerializable& object, const std::string_view json_string);
        static bool        save_to_file(const ISerializable& object, const std::filesystem::path filepath);
        static bool        load_from_file(ISerializable& object, const std::filesystem::path filepath);
    };
} // namespace codex
