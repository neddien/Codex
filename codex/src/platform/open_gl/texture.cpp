#include "texture.h"
#include "renderer.h"

#include <stb_image.h>
#include <stdexcept>

namespace codex::opengl {
    Texture::Texture(const u8* buf, usize len, TextureProperties properties)
        : props_(properties)
    {
        renderer_id_ = 0;
        buffer_      = nullptr;
        width_       = 0;
        height_      = 0;
        gpp_         = 0;

        GL_Call(glGenTextures(1, &renderer_id_));
        GL_Call(glBindTexture(GL_TEXTURE_2D, renderer_id_));

        stbi_set_flip_vertically_on_load(1);
        // buffer_ = stbi_load(str_path.c_str(), &width_, &height_, &gpp_, 4);
        buffer_ = stbi_load_from_memory(buf, len, &width_, &height_, &gpp_, 4);
        if (!buffer_) {
            throw std::runtime_error("[OpenGL]::[ERROR] >> Could not open texture file for reading: ");
        }

        // Since we are forcing 4 channels no matter what.
        properties.format = TextureFormat::RGBA8;

        // Set the wrap mode
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)properties.wrap_mode));
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)properties.wrap_mode));

        // Set the filter mode
        GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)properties.filter_mode));

        // Use mipmap filtering for minification and generate the actual mipmap.
        if (properties.mipmap_mode != TextureMipmapMode::None) {
            GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)properties.mipmap_mode));
            GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)properties.format, width_, height_, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, buffer_));
            GL_Call(glGenerateMipmap(GL_TEXTURE_2D));
        } else {
            GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)properties.filter_mode));
            GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)properties.format, width_, height_, 0, GL_RGBA,
                                 GL_UNSIGNED_BYTE, buffer_));
        }
        bind();
    }

    Texture::~Texture()
    {
        unbind();
        if (buffer_)
            stbi_image_free(buffer_);
        GL_Call(glDeleteTextures(1, &renderer_id_));
    }

    void Texture::bind(const u32 slot)
    {
        slot_ = GL_TEXTURE0 + slot;
        GL_Call(glActiveTexture(slot_));
        GL_Call(glBindTexture(GL_TEXTURE_2D, renderer_id_));
    }

    void Texture::unbind() const
    {
        GL_Call(glBindTexture(GL_TEXTURE_2D, 0));
    }

    GLenum Texture::format_type(TextureProperties props)
    {
        return (GLenum)0;
    }

} // namespace codex::opengl
