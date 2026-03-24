#pragma once

#include <codex.h>

#include "editor_panel.h"

namespace codex::editor {
    class FileSystem
    {
    public:
        class Directory;
        class File;

    private:
        std::filesystem::path  root_;
        std::vector<Directory> directories_;

    public:
        FileSystem(std::filesystem::path root);
    };

    class ContentBrowserView : public EditorPanel
    {
    public:
        using EditorPanel::EditorPanel;

    private:
        std::filesystem::path project_path_;

    protected:
        void on_init() override;
        void on_imgui_render() override;
    };

    class FileSystem::Directory
    {
    public:
        // Not sure if i will need this.
        enum class Type
        {
            User,
            Internal
        };

    private:
        std::filesystem::path                           path_;
        std::vector<mem::Shared<FileSystem::File>>      files_;
        std::vector<mem::Shared<FileSystem::Directory>> sub_directories_;

    public:
        Directory(std::filesystem::path path);
    };

    class FileSystem::File
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
            JsonFile,
            CXProjFile,
            ReflectFile,
            AudioFile,
            ImageFile,
            PrefabFile
        };

    private:
        std::filesystem::path path_;
        Type                  type_;

    public:
        File(std::filesystem::path path, const Type type = Type::Auto);

    public:
        [[nodiscard]] std::string get_name() const noexcept { return path_.filename().string(); }
        [[nodiscard]] std::string get_extension() const noexcept { return path_.extension().string(); }
    };
} // namespace codex::editor
