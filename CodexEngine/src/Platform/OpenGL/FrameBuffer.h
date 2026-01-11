#pragma once

#include "Constants.h"
#include "Texture.h"

namespace codex::opengl {
    struct FrameBufferProperties
    {
    public:
        uint32_t                       width;
        uint32_t                       height;
        uint32_t                       samples = 1;
        std::vector<TextureProperties> attachments;

    public:
        FrameBufferProperties() = default;
        FrameBufferProperties(const uint32_t width, const uint32_t height,
                              const std::initializer_list<TextureProperties> attachments)
            : width(width)
            , height(height)
            , attachments(attachments)
        {
        }
    };

    class CODEX_API FrameBuffer
    {
    private:
        uint32_t                       m_RendererId = 0;
        FrameBufferProperties          m_Properties;
        std::vector<TextureProperties> m_ColourAttachments;
        std::vector<uint32_t>          m_ColourAttachmentIds;
        TextureProperties              m_DepthAttachment;
        uint32_t                       m_DepthAttachmentId;

    public:
        FrameBuffer(const FrameBufferProperties& props);
        ~FrameBuffer();

    public:
        void                 Invalidate();
        void                 Bind();
        void                 Unbind();
        int                  ReadPixel(const uint32_t index, const int x, const int y);
        std::vector<uint8_t> ReadAllPixels(const uint32_t index)
        {
            std::vector<uint8_t> pixel_data(m_Properties.width * m_Properties.height * 4);
            GL_Call(glReadBuffer(GL_COLOR_ATTACHMENT0 + index));
            GL_Call(glReadPixels(0, 0, m_Properties.width, m_Properties.height, GL_RGBA, GL_UNSIGNED_BYTE,
                                 pixel_data.data()));
            return pixel_data;
        }
        void Resize(const uint32_t width, const uint32_t height);

    private:
        void   SetCurrentlyBoundTextureProperties(const TextureWrapMode wrapMode, const TextureFilterMode filterMode);
        GLenum DetermineFormatType(const TextureFormat format);

    public:
        inline uint32_t              GetId() const { return m_RendererId; }
        inline FrameBufferProperties GetProperties() const { return m_Properties; }
        inline uint32_t              GetColourAttachmentIdAt(const uint32_t index = 0) const
        {
            MGL_ASSERT(index < m_ColourAttachmentIds.size(), "Index outside bounds of colour attachments.");
            return m_ColourAttachmentIds[index];
        }
        inline uint32_t GetDepthAttachmentId() const { return m_DepthAttachmentId; }
        inline void     SetProperties(const FrameBufferProperties newProps)
        {
            m_Properties = newProps;
            Invalidate();
        }
        inline void SetReadOnly(const bool readOnly)
        {
            Bind();
            if (readOnly)
                glReadBuffer(GL_NONE);
            else
                glReadBuffer(GL_ONE);
            Unbind();
        }
        inline void SetDrawBuffer(const int attachmentId = 0)
        {
            Bind();
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachmentId);
            Unbind();
        }
    };
} // namespace codex::opengl
