#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/engine.h>
#include <engine/core/public/log.h>
#include <engine/filesystem/pak_mount.h>
#include <engine/utils/public/util.h>

namespace codex {
    constexpr u32 ProjectFormatVersion = 1;

    struct EngineProject : public ISerializable, public Loggable<"EngineProject">
    {
        // Global
        UUID        uuid;
        std::string name   = "Untitled";
        std::string author = "Codex Engine";
        std::string description;
        u32         format_ver = ProjectFormatVersion;
        struct EngineVer
        {
            u8  maj;
            u8  min;
            u8  rev;
            u16 build;

            [[nodiscard]] inline std::string to_string() const noexcept
            { return fmt::format("{}.{}.{}+{}", maj, min, rev, build); }
            [[nodiscard]] constexpr operator u64() const noexcept
            {
                return (u64)maj << (64 - 8) | (u64)min << (64 - 8 * 2) | (u64)rev << (64 - 8 * 3) |
                       (u64)build << (64 - 8 * 4);
            }
            constexpr void from_u64(u64 ver) noexcept
            {
                maj   = (u8)(ver >> (64 - 8));
                min   = (u8)(ver >> (64 - 8 * 2));
                rev   = (u8)(ver >> (64 - 8 * 3));
                build = (u16)(ver >> (64 - 8 * 4));
            }

        } engine_ver;

        // Editor: Editor only
        std::string assets_root = "assets/";
        std::string config_root = "config/";

        // Runtime: Default scene to launch on runtime
        AssetPath boot_scene;

        // Runtime: Engine properties
        EngineProperties engine_properties;

        // Runtime: NBMan shared libraries to load
        std::vector<std::string> native_modules;

        // Runtime: Additional mounts (DLCs and stuff)
        // std::vector<fs::PakMount> mounts;

        // Editor: Cook settings
        // CookSettings cook_settings;

    public:
        void archive(Archive& archive) override;

    public:
        void save_to_disk(const std::filesystem::path& proj_path) const;
        void save_to_vfs(fs::VirtualFilesystem& vfs, const std::filesystem::path& proj_path) const;

    public:
        [[nodiscard]] static Box<EngineProject> load_from_disk(const std::filesystem::path& proj_path);
        [[nodiscard]] static Box<EngineProject> load_from_vfs(fs::VirtualFilesystem& vfs, const std::string& proj_path);
    };

    // Applicable to the Editor only (.cxproj.user)
    struct EngineUserProject : public ISerializable, public Loggable<"EngineUserProject">
    {
    public:
        void archive(Archive& archive) override;
    };
} // namespace codex
