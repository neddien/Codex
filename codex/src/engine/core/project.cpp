#include "project.h"

#include <engine/core/public/binary_archive.h>

namespace codex {
    void EngineProject::archive(Archive& ar)
    {
        ar("uuid", uuid);
        ar("name", name);
        ar("author", author);
        ar("description", description);
        ar("format_ver", format_ver);

        if (ar.saving()) {
            u64 enginever64 = engine_ver;
            ar("engine_ver", enginever64);
        } else {
            u64 enginever64;
            ar("engine_ver", enginever64);
            engine_ver.from_u64(enginever64);
        }
        ar("assets_root", assets_root);
        ar("config_root", config_root);
        ar("boot_scene", boot_scene);
        ar("engine_properties", engine_properties);
        ar("native_modules", native_modules);
    }

    void EngineProject::save_to_disk(const std::filesystem::path& proj_path) const
    {
    }

    void EngineProject::save_to_vfs(fs::VirtualFilesystem& vfs, const std::filesystem::path& proj_path) const
    {
        auto fh = vfs.open(proj_path, { fs::FileMode::Create | fs::FileMode::Trunc | fs::FileMode::Write });
        if (fh) {
            BinaryArchiveBackend binsd;
            Archive              ar{ binsd };
            const_cast<EngineProject*>(this)->archive(ar);

            std::vector<u8> buffer = binsd.take_buffer();
            fh->write(buffer.data(), buffer.size());
        } else {
            throw IOException("{}: no such file or directory", proj_path.generic_string());
        }
    }

    Box<EngineProject> EngineProject::load_from_disk(const std::filesystem::path& proj_path)
    {
    }

    Box<EngineProject> EngineProject::load_from_vfs(fs::VirtualFilesystem& vfs, const std::string& proj_path)
    {
        auto fh = vfs.open(proj_path, { fs::FileMode::Read });
        if (fh) {
            auto eproj = Box<EngineProject>::make();

            std::vector<u8> buffer(fh->size());
            fh->read(buffer.data(), fh->size());

            BinaryArchiveBackend binsd{ buffer };
            Archive              ar{ binsd };
            eproj->archive(ar);

            return std::move(eproj);
        } else {
            throw IOException("{}: no such file or directory", proj_path);
        }
    }

    void EngineUserProject::archive(Archive& ar)
    {
    }
} // namespace codex
