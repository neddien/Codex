#include "scene_loader.h"

#include <engine/core/json_archive.h>
#include <engine/core/public/binary_archive.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/native_behaviour/public/native_behaviour.h>

namespace codex {
    Shared<Scene> SceneLoader::load(Shared<fs::FileHandle> fh, const Scene::ImportSettings& settings) const noexcept
    {
        auto scene = Shared<Scene>::make();

        switch (settings.ar_type()) {
            using enum Scene::ImportSettings::ArType;

            case kBinary: {
                std::vector<u8> buf;
                buf.resize(fh->size());
                fh->read(buf.data(), buf.size());
                SerializationManager::from_binary(*scene, buf);
            } break;
            case kJson: {
                std::string str_buf;
                str_buf.resize(fh->size());
                fh->read(str_buf.data(), str_buf.size());
                SerializationManager::from_json(*scene, str_buf);
            } break;
        }

        return scene;
    }
} // namespace codex
