#include "Texture.h"
#include "Renderer.h"

#include <stb_image.h>
#include <stdexcept>

namespace codex::opengl {
    Texture::Texture(std::filesystem::path filePath, TextureProperties properties)
        : m_FilePath(std::move(filePath))
        , m_Props(properties)
    {
        const auto str_path = m_FilePath.generic_string();

        m_RendererId = 0;
        m_Buffer     = nullptr;
        m_Width      = 0;
        m_Height     = 0;
        m_GPP        = 0;

        GL_Call(glGenTextures(1, &m_RendererId));
        GL_Call(glBindTexture(GL_TEXTURE_2D, m_RendererId));

        stbi_set_flip_vertically_on_load(1);
        m_Buffer = stbi_load(str_path.c_str(), &m_Width, &m_Height, &m_GPP, 4);
        if (!m_Buffer)
        {
            throw std::runtime_error("[OpenGL]::[ERROR] >> Could not open texture file for reading!");
        }

        // Since we are forcing 4 channels no matter what.
        properties.format = TextureFormat::RGBA8;

        // Set the wrap mode
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)properties.wrapMode));
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)properties.wrapMode));

        // Set the filter mode
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)properties.filterMode));

        // Use mipmap filtering for minification and generate the actual mipmap.
        if (properties.mipmapMode != TextureMipmapMode::None)
        {
            GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)properties.mipmapMode));
            GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)properties.format, m_Width, m_Height, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, m_Buffer));
            GL_Call(glGenerateMipmap(GL_TEXTURE_2D));
        }
        else
        {
            GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)properties.filterMode));
            GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)properties.format, m_Width, m_Height, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, m_Buffer));
        }
        Bind();
    }

    Texture::~Texture()
    {
        Unbind();
        if (m_Buffer)
            stbi_image_free(m_Buffer);
        GL_Call(glDeleteTextures(1, &m_RendererId));
    }

    void Texture::Bind(const uint32_t slot)
    {
        m_Slot = GL_TEXTURE0 + slot;
        GL_Call(glActiveTexture(m_Slot));
        GL_Call(glBindTexture(GL_TEXTURE_2D, m_RendererId));
    }

    void Texture::Unbind() const
    {
        GL_Call(glBindTexture(GL_TEXTURE_2D, 0));
    }

    GLenum Texture::GetFormatType(TextureProperties props)
    {
        return (GLenum)0;
    }

} // namespace codex::opengl
