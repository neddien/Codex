#include "public/serialization_manager.h"
#include "json_serializer.h"

#include <nlohmann/json.hpp>

namespace codex {

    std::string SerializationManager::serialize_to_json(const ISerializable& object)
    {
        nlohmann::ordered_json j;
        JsonSerializationNode  node{ j };
        object.serialize(node);
        return j.dump(4);
    }

    void SerializationManager::deserialize_from_json(ISerializable& object, const std::string_view json_string)
    {
        nlohmann::ordered_json j = nlohmann::json::parse(json_string);
        JsonSerializationNode  node{ j };
        object.deserialize(node);
    }

    bool SerializationManager::save_to_file(const ISerializable& object, const std::filesystem::path filepath)
    {
        std::ofstream file{ filepath };
        if (!file.is_open())
            return false;
        file << serialize_to_json(object);
        return true;
    }

    bool SerializationManager::load_from_file(ISerializable& object, const std::filesystem::path filepath)
    {
        std::ifstream file{ filepath };
        if (!file.is_open())
            return false;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        deserialize_from_json(object, content);
        return true;
    }
} // namespace codex