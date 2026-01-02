#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Serializer.h>

namespace codex {
    class SerializationManager
    {
    public:
        static std::string SerializeToJson(const ISerializable& object);
        static void        DeserializeFromJson(ISerializable& object, const std::string_view jsonString);
        static bool        SaveToFile(const ISerializable& object, const std::filesystem::path filepath);
        static bool        LoadFromFile(ISerializable& object, const std::filesystem::path filepath);
    };
} // namespace codex