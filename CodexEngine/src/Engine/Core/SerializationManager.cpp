#include "Public/SerializationManager.h"
#include "JsonSerializer.h"

#include <nlohmann/json.hpp>

namespace codex {

    std::string SerializationManager::SerializeToJson(const ISerializable& object)
    {
        nlohmann::ordered_json j;
        JsonSerializationNode  node{ j };
        object.Serialize(node);
        return j.dump(4);
    }

    void SerializationManager::DeserializeFromJson(ISerializable& object, const std::string_view json_string)
    {
        nlohmann::ordered_json j = nlohmann::json::parse(json_string);
        JsonSerializationNode  node{ j };
        object.Deserialize(node);
    }

    bool SerializationManager::SaveToFile(const ISerializable& object, const std::filesystem::path filepath)
    {
        std::ofstream file{ filepath };
        if (!file.is_open())
            return false;
        file << SerializeToJson(object);
        return true;
    }

    bool SerializationManager::LoadFromFile(ISerializable& object, const std::filesystem::path filepath)
    {
        std::ifstream file{ filepath };
        if (!file.is_open())
            return false;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        DeserializeFromJson(object, content);
        return true;
    }
} // namespace codex