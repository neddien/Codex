#ifndef CODEX_CORE_RESOURCE_HANDLER_H
#define CODEX_CORE_RESOURCE_HANDLER_H

#include "exception.h"
#include "i_resource.h"

#include <engine/graphics/public/shader.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    class ResourceException : public CodexException
    {
        using CodexException::CodexException;

    public:
        constexpr const char* default_message() const noexcept override { return "Bad resource."; }
    };

    class ResourceNotFoundException : public ResourceException
    {
        using ResourceException::ResourceException;

    public:
        constexpr const char* default_message() const noexcept override { return "Failed to load resource."; }
    };

    class CODEX_API Resources
    {
    private:
        static Resources* s_instance_;

    private:
        std::unordered_map<usize, ResRef<IResource>> resources_;

    public:
        static void init();
        static void destroy();

    private:
        static ResRef<gfx::Texture2D> load_texture_2d(const std::filesystem::path     filePath,
                                                      const opengl::TextureProperties props = {});
        static ResRef<gfx::Shader>    load_shader(const std::filesystem::path filePath,
                                                  const std::string_view      version = "330 core");

    public:
        template <typename T, typename... TArgs>
        static ResRef<T> load(std::filesystem::path filePath, TArgs&&... args)
        {
            using namespace codex::gfx;

            if constexpr (std::is_same_v<T, Texture2D>)
                return load_texture_2d(std::move(filePath), std::forward<TArgs>(args)...);
            else if constexpr (std::is_same_v<T, Shader>)
                return load_shader(std::move(filePath), std::forward<TArgs>(args)...);

            static_assert("Type not supported.");
            return nullptr;
        }
        template <typename T>
        static ResRef<T> from(T&& resource)
        {
            auto res = mem::Shared<T>::make(std::move(resource));

            if (has_resource(res->path())) {
                // throw ResourceException("Resource with the same path already exists.");
            }

            const usize id = util::crypto::djb2_hash(res->path().generic_string());
            lgx::Get("engine").Log(lgx::Info, "[ResourceHandler] >> File: '{}' Id: {}", res->path().string(), id);
            s_instance_->resources_[id] = res;

            return res;
        }

    public:
        template <typename T>
        static ResRef<T> get(const usize id)
        {
            auto it = s_instance_->resources_.find(id);
            if (it != s_instance_->resources_.end())
                return it->second.as<T>();
            else
                throw ResourceNotFoundException(
                    fmt::format("Hash Id {} was not present in the resource pool.", id).c_str());
        }
        template <typename T>
        static ResRef<T> get(const std::filesystem::path filePath)
        {
            return get<T>(util::crypto::djb2_hash(filePath.generic_string()));
        }
        static bool                                          has_resource(const usize id);
        static bool                                          has_resource(const std::filesystem::path filePath);
        static std::unordered_map<usize, ResRef<IResource>>& resources();
    };
} // namespace codex

#endif // CODEX_CORE_RESOURCE_HANDLER_H
