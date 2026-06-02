#include "public/serialization_manager.h"

#include "json_archive.h"
#include "public/binary_archive.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace codex {
    std::string SerializationManager::to_json(const ISerializable& object)
    {
        nlohmann::ordered_json root;
        JsonArchiveBackend     backend{ root, /*saving=*/true };
        Archive                archive{ backend };
        const_cast<ISerializable&>(object).archive(archive); // save mode does not mutate
        return root.dump(4);
    }

    void SerializationManager::from_json(ISerializable& object, const std::string_view json)
    {
        nlohmann::ordered_json root;
        try {
            root = nlohmann::json::parse(json);
        }
        catch (const std::exception& e) {
            throw DeserializationException("Failed to parse JSON: {}", e.what());
        }
        JsonArchiveBackend backend{ root, /*saving=*/false };
        Archive            archive{ backend };
        object.archive(archive);
    }

    std::vector<u8> SerializationManager::to_binary(const ISerializable& object)
    {
        BinaryArchiveBackend backend; // save mode
        Archive              archive{ backend };
        const_cast<ISerializable&>(object).archive(archive);
        return backend.take_buffer();
    }

    void SerializationManager::from_binary(ISerializable& object, std::span<const u8> data)
    {
        BinaryArchiveBackend backend{ data };
        Archive              archive{ backend };
        object.archive(archive);
    }

    bool SerializationManager::save_to_file(const ISerializable& object, const std::filesystem::path& filepath,
                                            const Format format)
    {
        const std::filesystem::path tmp = std::filesystem::path{ filepath } += ".tmp";

        switch (format) {
            using enum Format;
            case Json: {
                std::ofstream file{ tmp, std::ios::binary | std::ios::trunc };
                if (!file.is_open())
                    return false;
                file << to_json(object);
            } break;
            case Binary: {
                std::ofstream file{ tmp, std::ios::binary | std::ios::trunc };
                if (!file.is_open())
                    return false;
                const std::vector<u8> bytes = to_binary(object);
                file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            } break;
        }

        std::error_code ec;
        std::filesystem::rename(tmp, filepath, ec); // atomic replace
        if (ec) {
            std::filesystem::remove(tmp, ec);
            return false;
        }
        return true;
    }

    bool SerializationManager::load_from_file(ISerializable& object, const std::filesystem::path& filepath,
                                              const Format format)
    {
        std::ifstream file{ filepath, std::ios::binary };
        if (!file.is_open())
            return false;

        switch (format) {
            using enum Format;
            case Json: {
                const std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                from_json(object, content);
            } break;
            case Binary: {
                const std::vector<u8> bytes{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                from_binary(object, bytes);
            } break;
        }

        return true;
    }
} // namespace codex
