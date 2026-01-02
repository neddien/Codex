#include "FrameBuffer.h"

namespace codex::opengl {
    namespace {
        constexpr auto MAX_FRAME_BUFFER_SIZE       = 8192;
        constexpr auto MAX_COLOUR_ATTACHMENT_COUNT = 4;

        namespace Internals {
            bool IsDepthFormat(const TextureFormat format)
            {
                switch (format)
                {
                    case TextureFormat::Depth:
                    case TextureFormat::Depth16:
                    case TextureFormat::Depth24:
                    case TextureFormat::Depth32:
                    case TextureFormat::Depth32F:
                    case TextureFormat::Depth24Stencil8:
                    case TextureFormat::Depth32FStencil8: return true;
                    default: return false;
                }
            }

            GLenum GetDepthAttachmentTypeFromFormat(const TextureFormat format)
            {
                switch (format)
                {
                    case TextureFormat::Depth:
                    case TextureFormat::Depth16:
                    case TextureFormat::Depth24:
                    case TextureFormat::Depth32:
                    case TextureFormat::Depth32F: return GL_DEPTH_ATTACHMENT;
                    case TextureFormat::Depth24Stencil8:
                    case TextureFormat::Depth32FStencil8: return GL_DEPTH_STENCIL_ATTACHMENT;
                    default: MGL_ASSERT(false, "Bad texture format."); return (GLenum)GL_INVALID_ENUM;
                }
            }

            GLenum GetFormatFromInternalFormat(const TextureFormat format)
            {
                switch (format)
                {
                    case TextureFormat::Depth:
                    case TextureFormat::Depth16:
                    case TextureFormat::Depth24:
                    case TextureFormat::Depth32:
                    case TextureFormat::Depth32F:
                    case TextureFormat::Depth24Stencil8:
                    case TextureFormat::Depth32FStencil8:
                    case TextureFormat::RGBA8:
                    case TextureFormat::RGB32F: return GL_RGBA;
                    case TextureFormat::RedInt32:
                    case TextureFormat::RedUInt32:
                    case TextureFormat::RedFloat32: return GL_RED_INTEGER;
                    default: return GL_INVALID_ENUM;
                }
            }

            GLenum GetTypeFromInternalFormat(const TextureFormat format)
            {
                switch (format)
                {
                    case TextureFormat::RGBA8: return GL_UNSIGNED_BYTE;
                    case TextureFormat::RedInt32: return GL_INT;
                    case TextureFormat::RedUInt32: return GL_UNSIGNED_INT;
                    case TextureFormat::RedFloat32: return GL_FLOAT;
                    default: return GL_INVALID_ENUM;
                }
            }

            void AttachColourTexture(const uint32_t id, const TextureProperties& props, const uint32_t width,
                                     const uint32_t height, const size_t index)
            {
                GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)props.format, width, height, 0,
                                     GetFormatFromInternalFormat(props.format),
                                     Internals::GetTypeFromInternalFormat(props.format), nullptr));

                // Set the wrap mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)props.wrapMode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)props.wrapMode));

                // Set the filter mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)props.filterMode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)props.filterMode));

                GL_Call(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, id, 0));
            }

            void AttachDepthTexture(const uint32_t id, const TextureProperties& props, const uint32_t width,
                                    const uint32_t height)
            {
                GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)props.format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                     nullptr));

                // Set the wrap mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)props.wrapMode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)props.wrapMode));

                // Set the filter mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)props.filterMode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)props.filterMode));

                GL_Call(glFramebufferTexture2D(GL_FRAMEBUFFER, GetDepthAttachmentTypeFromFormat(props.format),
                                               GL_TEXTURE_2D, id, 0));
            }
        } // namespace Internals
    } // namespace

    FrameBuffer::FrameBuffer(const FrameBufferProperties& props)
        : m_Properties(props)
    {
        for (const auto& tex_props : props.attachments)
        {
            if (Internals::IsDepthFormat(tex_props.format))
                m_DepthAttachment = tex_props;
            else
                m_ColourAttachments.push_back(tex_props);
        }

        Invalidate();
    }

    FrameBuffer::~FrameBuffer()
    {
        GL_Call(glDeleteFramebuffers(1, &m_RendererId));
        GL_Call(glDeleteTextures(m_ColourAttachmentIds.size(), m_ColourAttachmentIds.data()));
        GL_Call(glDeleteTextures(1, &m_DepthAttachmentId));
    }

    void FrameBuffer::Invalidate()
    {
        if (m_RendererId)
        {
            GL_Call(glDeleteFramebuffers(1, &m_RendererId));
            GL_Call(glDeleteTextures(m_ColourAttachmentIds.size(), m_ColourAttachmentIds.data()));
            GL_Call(glDeleteTextures(1, &m_DepthAttachmentId));

            m_ColourAttachmentIds.clear();
            m_DepthAttachment   = TextureProperties{};
            m_DepthAttachmentId = 0;
        }

        GL_Call(glGenFramebuffers(1, &m_RendererId));
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererId));

        if (!m_ColourAttachments.empty())
        {
            m_ColourAttachmentIds.resize(m_ColourAttachments.size());
            GL_Call(glGenTextures(m_ColourAttachmentIds.size(), m_ColourAttachmentIds.data()));

            for (size_t i = 0; i < m_ColourAttachmentIds.size(); ++i)
            {
                glBindTexture(GL_TEXTURE_2D, m_ColourAttachmentIds[i]);
                Internals::AttachColourTexture(m_ColourAttachmentIds[i], m_ColourAttachments[i], m_Properties.width,
                                               m_Properties.height, i);
            }
        }

        if (m_DepthAttachment.format != TextureFormat::None)
        {
            GL_Call(glGenTextures(1, &m_DepthAttachmentId));
            GL_Call(glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentId));

            switch (m_DepthAttachment.format)
            {
                case TextureFormat::Depth24Stencil8:
                    Internals::AttachDepthTexture(m_DepthAttachmentId, m_DepthAttachment, m_Properties.width,
                                                  m_Properties.height);
                    break;
                default: break;
            }
        }

        if (!m_ColourAttachments.empty())
        {
            GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererId));
            MGL_ASSERT(m_ColourAttachments.size() < MAX_COLOUR_ATTACHMENT_COUNT,
                       "Cannot have more than " + std::to_string(MAX_COLOUR_ATTACHMENT_COUNT) + " colour attachments.");
            MGL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                       "Error: Framebuffer is not complete.");

            GLenum buffers[MAX_COLOUR_ATTACHMENT_COUNT];
            for (int i = 0; i < MAX_COLOUR_ATTACHMENT_COUNT; ++i)
                buffers[i] = GL_COLOR_ATTACHMENT0 + i;

            GL_Call(glDrawBuffers(MAX_COLOUR_ATTACHMENT_COUNT, buffers));
        }
        else
        {
            glDrawBuffer(GL_NONE);
        }
    }

    void FrameBuffer::Bind()
    {
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, m_RendererId));
        GL_Call(glViewport(0, 0, m_Properties.width, m_Properties.height));
    }

    void FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Resize(const uint32_t width, const uint32_t height)
    {
        if (width != 0 && height != 0 && width < MAX_FRAME_BUFFER_SIZE && height < MAX_FRAME_BUFFER_SIZE)
        {
            m_Properties.width  = width;
            m_Properties.height = height;
            Invalidate();
        }
    }

    int FrameBuffer::ReadPixel(const uint32_t index, const int x, const int y)
    {
        MGL_ASSERT(index < m_ColourAttachmentIds.size(), "Index outside bounds of attachments.");
        GL_Call(glReadBuffer(GL_COLOR_ATTACHMENT0 + index));
        int pixel_data;
        GL_Call(glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel_data));
        return pixel_data;
    }

    void FrameBuffer::SetCurrentlyBoundTextureProperties(const TextureWrapMode   wrapMode,
                                                         const TextureFilterMode filterMode)
    {
        switch (wrapMode)
        {
            case TextureWrapMode::Mirror:
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
                break;
            case TextureWrapMode::Stretch:
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                break;
            case TextureWrapMode::Border:
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
                break;
        }

        switch (filterMode)
        {
            case TextureFilterMode::Linear:
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                break;
            case TextureFilterMode::Nearest:
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
                break;
        }
    }

    GLenum FrameBuffer::DetermineFormatType(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::Depth24Stencil8: return GL_UNSIGNED_INT_24_8;
            case TextureFormat::RGB32F: return GL_FLOAT;
            case TextureFormat::RGBA8: return GL_UNSIGNED_BYTE;
            default: MGL_ASSERT(false, "Bad texture format.");
        }
        return (GLenum)0;
    }
} // namespace codex::opengl
