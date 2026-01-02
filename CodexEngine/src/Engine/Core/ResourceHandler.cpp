#include "Public/ResourceHandler.h"

#include <Engine/Graphics/Public/Shader.h>
#include <Engine/Graphics/Public/Texture2D.h>

namespace codex {
    using namespace codex::gfx;

    Resources* Resources::m_Instance = nullptr;

    void Resources::Init()
    {
        if (!m_Instance)
        {
            m_Instance = new Resources();

            lgx::Get("engine").Log(lgx::Info, "ResourceHandler subsystem initialized.");
        }
    }

    void Resources::Destroy()
    {
        if (m_Instance)
        {
            delete m_Instance;
            m_Instance = nullptr;

            lgx::Get("engine").Log(lgx::Info, "ResourceHandler subsystem disposed.");
        }
    }

    ResRef<Texture2D> Resources::Load_Texture2D(const std::filesystem::path filePath, const opengl::TextureProperties props)
    {
        // I could use .c_str() but on MSVC c_str() is of value_type type which itself is wchar_t.
        // To put it short, It is for compatability reasons.
        if (HasResource(filePath))
            return GetResource<Texture2D>(filePath);

        // TODO: Use std::filesystem::path instead of string_view.
        std::ifstream fs(filePath);
        if (fs.is_open())
        {
            usize id = util::Crypto::DJB2Hash(filePath.string());

            ResRef<Texture2D> texture   = mem::Shared<Texture2D>::New(filePath, props);
            m_Instance->m_Resources[id] = texture;
            fs.close();
            lgx::Get("engine").Log(lgx::Info, "[ResourceHandler] >> File: '{}' Id: {}", filePath.string(), id);
            return texture;
        }
        else
        {
            cx_throw(ResourceNotFoundException, "Couldn't open file '{}' for reading. Failed to load texture asset.",
                     filePath.string());
            return nullptr;
        }
    }

    ResRef<Shader> Resources::Load_Shader(const std::filesystem::path filePath, const std::string_view version)
    {
        if (HasResource(filePath))
            return GetResource<Shader>(filePath);

        // TODO: Use std::filesystem::path instead of string_view.
        std::ifstream fs(filePath.string());
        if (fs.is_open())
        {
            const usize    id           = util::Crypto::DJB2Hash(filePath.string());
            ResRef<Shader> shader       = mem::Shared<Shader>::New(filePath, version);
            m_Instance->m_Resources[id] = shader;
            fs.close();
            lgx::Get("engine").Log(lgx::Info, "[ResourceHandler] >> File: '{}' Id: {}", filePath.string(), id);
            return shader;
        }
        else
        {
            cx_throw(ResourceNotFoundException, "Couldn't open file '{}' for reading. Failed to load shader asset.",
                     filePath.string());
            return nullptr;
        }
    }

    bool Resources::HasResource(const usize id)
    {
        auto it = m_Instance->m_Resources.find(id);
        if (it != m_Instance->m_Resources.end())
            return true;
        return false;
    }

    bool Resources::HasResource(const std::filesystem::path filePath)
    {
        const usize id = util::Crypto::DJB2Hash(filePath.string());
        return HasResource(id);
    }

    std::unordered_map<usize, ResRef<IResource>>& Resources::GetAllResources()
    {
        return m_Instance->m_Resources;
    }
} // namespace codex
