#include "public/resource_handler.h"

#include <engine/graphics/public/shader.h>
#include <engine/graphics/public/texture2d.h>

namespace codex {
    using namespace codex::gfx;

    Resources* Resources::s_instance_ = nullptr;

    void Resources::init()
    {
        if (!s_instance_) {
            s_instance_ = new Resources();

            info("ResourceHandler subsystem initialized.");
        }
    }

    void Resources::destroy()
    {
        if (s_instance_) {
            delete s_instance_;
            s_instance_ = nullptr;

            info("ResourceHandler subsystem disposed.");
        }
    }

    ResRef<Texture2D> Resources::load_texture_2d(const std::filesystem::path     file_path,
                                                 const opengl::TextureProperties props)
    {
        // I could use .c_str() but on MSVC c_str() is of value_type type which itself is wchar_t.
        // To put it short, It is for compatability reasons.
        if (has_resource(file_path))
            return get<Texture2D>(file_path);

        // TODO: Use std::filesystem::path instead of string_view.
        std::ifstream fs(file_path);
        if (fs.is_open()) {
            usize id = util::crypto::djb2_hash(file_path.generic_string());

            ResRef<Texture2D> texture   = mem::Shared<Texture2D>::make(file_path, props);
            s_instance_->resources_[id] = texture;
            fs.close();
            info("[ResourceHandler] >> File: '{}' Id: {}", file_path.string(), id);
            return texture;
        } else {
            throw ResourceNotFoundException("Couldn't open file '{}' for reading. Failed to load texture asset.",
                                            file_path.string());
            return nullptr;
        }
    }

    ResRef<Shader> Resources::load_shader(const std::filesystem::path file_path, const std::string_view version)
    {
        if (has_resource(file_path))
            return get<Shader>(file_path);

        // TODO: Use std::filesystem::path instead of string_view.
        std::ifstream fs(file_path.string());
        if (fs.is_open()) {
            const usize    id           = util::crypto::djb2_hash(file_path.generic_string());
            ResRef<Shader> shader       = mem::Shared<Shader>::make(file_path, version);
            s_instance_->resources_[id] = shader;
            fs.close();
            info("[ResourceHandler] >> File: '{}' Id: {}", file_path.string(), id);
            return shader;
        } else {
            throw ResourceNotFoundException("Couldn't open file '{}' for reading. Failed to load shader asset.",
                                            file_path.string());
            return nullptr;
        }
    }

    bool Resources::has_resource(const usize id)
    {
        auto it = s_instance_->resources_.find(id);
        if (it != s_instance_->resources_.end())
            return true;
        return false;
    }

    bool Resources::has_resource(const std::filesystem::path file_path)
    {
        const usize id = util::crypto::djb2_hash(file_path.generic_string());
        return has_resource(id);
    }

    std::unordered_map<usize, ResRef<IResource>>& Resources::resources()
    {
        return s_instance_->resources_;
    }
} // namespace codex
