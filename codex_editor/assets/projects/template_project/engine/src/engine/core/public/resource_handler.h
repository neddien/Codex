#pragma once

#include "exception.h"
#include "i_resource.h"

#include <engine/graphics/public/shader.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    class ResourceException : public CodexException
    {
        using CodexException::CodexException;

    public:
        [[nodiscard]] constexpr const char* default_message() const noexcept override { return "Bad resource."; }
    };

    class ResourceNotFoundException : public ResourceException
    {
        using ResourceException::ResourceException;

    public:
        [[nodiscard]] constexpr const char* default_message() const noexcept override
        {
            return "Failed to load resource.";
        }
    };

    class CODEX_API Resources
    {
    public:
        static void init();
        static void destroy();

        template <typename T, typename... TArgs>
        [[nodiscard]] static ResRef<T> load(std::filesystem::path file_path, TArgs&&... args);

        template <typename T>
        static ResRef<T> from(T&& resource);

        template <typename T>
        [[nodiscard]] static ResRef<T> get(const usize id);

        template <typename T>
        [[nodiscard]] static ResRef<T> get(const std::filesystem::path file_path);

        [[nodiscard]] static bool has_resource(const usize id);
        [[nodiscard]] static bool has_resource(const std::filesystem::path file_path);
        [[nodiscard]] static std::unordered_map<usize, ResRef<IResource>>& resources();

    private:
        static ResRef<gfx::Texture2D> load_texture_2d(const std::filesystem::path     file_path,
                                                      const opengl::TextureProperties props = {});
        static ResRef<gfx::Shader>    load_shader(const std::filesystem::path file_path,
                                                  const std::string_view      version = "330 core");

    private:
        static Resources* s_instance_;

    private:
        std::unordered_map<usize, ResRef<IResource>> resources_;
    };
} // namespace codex

#include "resource_handler.inl"
