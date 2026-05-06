#pragma once

#include "constants.h"
#include "texture.h"

namespace codex::opengl {
    struct FrameBufferProperties
    {
    public:
        u32                            width;
        u32                            height;
        u32                            samples = 1;
        std::vector<TextureProperties> attachments;

    public:
        FrameBufferProperties() = default;
        FrameBufferProperties(const u32 width, const u32 height,
                              const std::initializer_list<TextureProperties> attachments)
            : width(width)
            , height(height)
            , attachments(attachments)
        {
        }
    };

    class CODEX_API FrameBuffer
    {
    public:
        explicit FrameBuffer(const FrameBufferProperties& props);
        ~FrameBuffer();

    public:
        [[nodiscard]] inline u32                   id() const { return renderer_id_; }
        [[nodiscard]] inline FrameBufferProperties properties() const { return props_; }
        [[nodiscard]] inline u32                   colour_attachment_id_at(const u32 index = 0) const
        {
            CX_ASSERT(index < colour_attachment_ids_.size(), "Index outside bounds of colour attachments.");
            return colour_attachment_ids_[index];
        }
        [[nodiscard]] inline u32 depth_attachment_id() const { return depth_attachment_id_; }

    public:
        void                 invalidate();
        void                 bind();
        void                 unbind();
        int                  read_pixel(const u32 index, const i32 x, const i32 y);
        std::vector<u8>      read_all_pixels(const u32 index)
        {
            std::vector<u8> pixel_data(props_.width * props_.height * 4);
            GL_Call(glReadBuffer(GL_COLOR_ATTACHMENT0 + index));
            GL_Call(glReadPixels(0, 0, props_.width, props_.height, GL_RGBA, GL_UNSIGNED_BYTE,
                                 pixel_data.data()));
            return pixel_data;
        }
        void resize(const u32 width, const u32 height);
        inline void set_properties(const FrameBufferProperties new_props)
        {
            props_ = new_props;
            invalidate();
        }
        inline void set_read_only(const bool read_only)
        {
            bind();
            if (read_only)
                glReadBuffer(GL_NONE);
            else
                glReadBuffer(GL_ONE);
            unbind();
        }
        inline void set_draw_buffer(const i32 attachment_id = 0)
        {
            bind();
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment_id);
            unbind();
        }

    private:
        void   set_currently_bound_texture_properties(const TextureWrapMode wrap_mode, const TextureFilterMode filter_mode);
        GLenum determine_format_type(const TextureFormat format);

    private:
        u32                            renderer_id_ = 0;
        FrameBufferProperties          props_;
        std::vector<TextureProperties> colour_attachments_;
        std::vector<u32>               colour_attachment_ids_;
        TextureProperties              depth_attachment_;
        u32                            depth_attachment_id_;
    };
} // namespace codex::opengl
