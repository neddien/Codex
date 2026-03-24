#pragma once

#include "resource_handler.h"

namespace codex {
    template <typename T, typename... TArgs>
    ResRef<T> Resources::load(std::filesystem::path file_path, TArgs&&... args)
    {
        using namespace codex::gfx;

        if constexpr (std::is_same_v<T, Texture2D>)
            return load_texture_2d(std::move(file_path), std::forward<TArgs>(args)...);
        else if constexpr (std::is_same_v<T, Shader>)
            return load_shader(std::move(file_path), std::forward<TArgs>(args)...);

        static_assert(std::is_same_v<T, Texture2D> || std::is_same_v<T, Shader>, "Type not supported.");
        return nullptr;
    }

    template <typename T>
    ResRef<T> Resources::from(T&& resource)
    {
        auto res = mem::Shared<T>::make(std::move(resource));

        if (has_resource(res->path())) {
            // throw ResourceException("Resource with the same path already exists.");
        }

        const usize id = util::crypto::djb2_hash(res->path().generic_string());
        info("[ResourceHandler] >> File: '{}' Id: {}", res->path().string(), id);
        s_instance_->resources_[id] = res;

        return res;
    }

    template <typename T>
    ResRef<T> Resources::get(const usize id)
    {
        auto it = s_instance_->resources_.find(id);
        if (it != s_instance_->resources_.end())
            return it->second.as<T>();
        else
            throw ResourceNotFoundException(
                fmt::format("Hash Id {} was not present in the resource pool.", id).c_str());
    }

    template <typename T>
    ResRef<T> Resources::get(const std::filesystem::path file_path)
    {
        return get<T>(util::crypto::djb2_hash(file_path.generic_string()));
    }
} // namespace codex
