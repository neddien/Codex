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

            [[nodiscard]] constexpr std::string to_string() const noexcept
            {
                return fmt::format("{}.{}.{}+{}", maj, min, rev, build);
            }
        } engine_ver;

        // Editor: Editor only
        std::string assets_root = "assets/";
        std::string config_root = "config/";

        // Runtime: Default scene to launch on runtime
        AssetPath boot_scene;

        // Runtime: Engine properties
        EngineProperties engine_properties;
        f32              fixed_tick_rate = 60.0f;

        // Runtime: NBMan shared libraries to load
        std::vector<std::string> native_modules;

        // Runtime: Additional mounts (DLCs and stuff)
        std::vector<fs::PakMount> mounts;

        // Editor: Cook settings
        // CookSettings cook_settings;

    public:
        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;

    public:
        void save_to_disk(const std::filesystem::path& proj_path);
        void save_to_vfs(fs::VirtualFilesystem& vfs, const std::filesystem::path& proj_path);

    public:
        [[nodiscard]] static Box<EngineProject> load_from_disk(const std::filesystem::path& proj_path);
        [[nodiscard]] static Box<EngineProject> load_from_vfs(fs::VirtualFilesystem&       vfs,
                                                              const std::filesystem::path& proj_path);
    };

    // Applicable to the Editor only (.cxproj.user)
    struct EngineUserProject : public ISerializable, public Loggable<"EngineUserProject">
    {
        AssetPath   last_open_scene;
        std::string last_asset_path;

    public:
        void serialize(ISerializationNode& node) const override;
        void deserialize(const ISerializationNode& node) override;
    };
} // namespace codex
