#include "public/asset_registry.h"

#include <engine/core/engine.h>
#include <engine/filesystem/vfs.h>

namespace codex {
    using namespace codex::cc;
    using namespace codex::fs;

    Task<void> AssetRegistry::scan(fs::VirtualFilesystem& vfs, const std::string& path)
    {
        auto checksum_file = [&](FileHandle& handle) -> u32

        {
            constexpr usize chunk_size = 64 * 1024; // 64KiB
            std::vector<u8> buf(chunk_size);
            u32             crc = 0xFFFFFFFFu;
            usize           n   = 0;

            while ((n = handle.read(buf.data(), chunk_size)) > 0) {
                for (usize i = 0; i < n; ++i) {
                    crc ^= buf[i];
                    for (int k = 0; k < 8; ++k)
                        crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
                }
            }

            return ~crc;
        };

        auto metagen = [&](const std::string& path) -> Task<AssetMetadata>
        {
            co_await Engine::get_worker_pool();

            AssetMetadata metadata{};

            metadata.path = AssetPath{
                .uuid = UUID{},
                .path = path,
            };
            metadata.type = "None";

            const auto ext    = util::str::split(path, '.').back();
            auto       loader = AssetManager::get_loader_by_ext(ext);

            if (!loader) {
                error("No loader associated with extension: {}", ext);
                co_return AssetMetadata{}; // TODO: Perhaps std::optional?
            }

            metadata.type = loader->asset_type();

            auto handle = vfs.open(path, { FileMode::Read });
            if (handle) {
                metadata.checksum = checksum_file(*handle);
            } else {
                error("Handle for file: {} is invalid", path);
            }

            co_return metadata;
        };

        if (vfs.exists(path)) {
            if (vfs.is_directory(path)) {
                std::vector<std::string> list = vfs.list(path, ListOptions::FilesOnly | ListOptions::Recursive);

                std::vector<AssetMetadata>       metadatas;
                std::vector<Task<AssetMetadata>> tasks;
                tasks.reserve(list.size());

                // grabbing the appropriate loader for the asset:
                // * loader should be grabbed from Asset Type

                // does file have a metadata entry?
                // if no then create one which makes the following assumption:
                // * the extension of the file matches the file type e.g., a png file is infact a png image file
                // * the loader can load this file without the requirement of any additional loading parameters
                // * if these assumtions are met then:
                // ** grab the loader based on the file extension: auto loader = get_loader_from_ext(file.extension());
                // ** load the asset with default load parameters: auto asset = loader.load(fh);
                // else:
                // * "import_settings" field exists and is deserializable or is simply empty.
                // * get the load based on metadata.type: auto loader = get_loader(metadata.type);
                // * load the asset: auto asset = loader.load(fh, serdes_node);

                for (const auto& ipath : list) {
                    auto task = metagen(ipath);
                    tasks.push_back(std::move(task));

                    for (auto& task : tasks) {
                        metadatas.push_back(co_await task);
                    }

                    metas_ = std::move(metadatas);
                }
            } else {
                error("AssetRegistry: {}: no such file or directory", path);
            }
        }
    }

    void AssetMetadata::serialize(ISerializationNode& node) const
    {
    }

    void AssetMetadata::deserialize(const ISerializationNode& node)
    {
    }
} // namespace codex
