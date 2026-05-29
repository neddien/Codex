#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class ContentBrowserView : public EditorPanel, public Loggable<"ContentBrowserView">
    {
    private:
        struct TreeNode
        {
            std::string                               name;
            std::unordered_map<std::string, TreeNode> children;
        };

    public:
        using EditorPanel::EditorPanel;

    protected:
        void on_init() override;
        void on_imgui_render() override;
        void on_update(const f32 dt) override;

    public:
        void             chdir_nolock(const std::string& path);
        void             chdir(const std::string& path);
        void             refresh_nolock(const std::string& path);
        void             refresh(const std::string& path);
        std::string_view cwd() const;

    private:
        void rebuild_tree();
        void render_directory_tree(const TreeNode& node, std::string path);
        void render_asset_grid();
        void render_breadcrumb();
        void render_icon_size_slider();
        void render_asset_based_on_type(const AssetMetadata& meta, const ImVec2 screen_pos) noexcept;

    private:
        std::unordered_map<UUID, Shared<void>> asset_cache_;
        std::string                            current_path_;
        std::string                            root_path_;
        UUID                                   selected_;
        f32                                    icon_size_ = 80.0f;
        bool                                   dirty_     = true;
        std::vector<AssetMetadata>             cache_;
        mutable std::shared_mutex              mutex_;
        mutable TreeNode                       root_node_;
    };

    class File
    {
    public:
        enum class Type
        {
            // TODO: Expand AudioFile and ImageFile e.g., MP3File, WAVFile, PNGFile, JPEGFile...
            Auto, // FileSystem::File will detect the file type, for now, it will detect based off of extensions (naive
                  // approach).
            GenericFile,
            TextFile,
            CxxFile,
            CSFile,
            LuaFile,
            JsonFile,
            CXProjFile,
            ReflectFile,
            AudioFile,
            FModFile,
            ImageFile,
            PrefabFile
        };

    private:
        std::filesystem::path path_;
        Type                  type_;

    public:
        File(std::filesystem::path path, const Type type = Type::Auto);

    public:
        [[nodiscard]] std::string name() const noexcept { return path_.filename().string(); }
        [[nodiscard]] std::string extension() const noexcept { return path_.extension().string(); }
    };
} // namespace codex::editor
