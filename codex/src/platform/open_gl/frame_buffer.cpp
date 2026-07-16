#include "frame_buffer.h"

namespace codex::opengl {
    namespace {
        constexpr auto MAX_FRAME_BUFFER_SIZE       = 8192;
        constexpr auto MAX_COLOUR_ATTACHMENT_COUNT = 4;

        namespace internals {
            bool is_depth_format(const TextureFormat format)
            {
                switch (format) {
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

            GLenum get_depth_attachment_type_from_format(const TextureFormat format)
            {
                switch (format) {
                    case TextureFormat::Depth:
                    case TextureFormat::Depth16:
                    case TextureFormat::Depth24:
                    case TextureFormat::Depth32:
                    case TextureFormat::Depth32F: return GL_DEPTH_ATTACHMENT;
                    case TextureFormat::Depth24Stencil8:
                    case TextureFormat::Depth32FStencil8: return GL_DEPTH_STENCIL_ATTACHMENT;
                    default: cxassert(false, "Bad texture format."); return (GLenum)GL_INVALID_ENUM;
                }
            }

            GLenum get_format_from_internal_format(const TextureFormat format)
            {
                switch (format) {
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

            GLenum get_type_from_internal_format(const TextureFormat format)
            {
                switch (format) {
                    case TextureFormat::RGBA8: return GL_UNSIGNED_BYTE;
                    case TextureFormat::RedInt32: return GL_INT;
                    case TextureFormat::RedUInt32: return GL_UNSIGNED_INT;
                    case TextureFormat::RedFloat32: return GL_FLOAT;
                    default: return GL_INVALID_ENUM;
                }
            }

            void attach_colour_texture(const u32 id, const TextureProperties& props, const u32 width, const u32 height,
                                       const usize index)
            {
                GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)props.format, width, height, 0,
                                     get_format_from_internal_format(props.format),
                                     internals::get_type_from_internal_format(props.format), nullptr));

                // Set the wrap mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)props.wrap_mode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)props.wrap_mode));

                // Set the filter mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)props.filter_mode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)props.filter_mode));

                GL_Call(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, id, 0));
            }

            void attach_depth_texture(const u32 id, const TextureProperties& props, const u32 width, const u32 height)
            {
                GL_Call(glTexImage2D(GL_TEXTURE_2D, 0, (GLint)props.format, width, height, 0, GL_DEPTH_STENCIL,
                                     GL_UNSIGNED_INT_24_8, nullptr));

                // Set the wrap mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)props.wrap_mode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)props.wrap_mode));

                // Set the filter mode
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)props.filter_mode));
                GL_Call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)props.filter_mode));

                GL_Call(glFramebufferTexture2D(GL_FRAMEBUFFER, get_depth_attachment_type_from_format(props.format),
                                               GL_TEXTURE_2D, id, 0));
            }
        } // namespace internals
    } // namespace

    FrameBuffer::FrameBuffer(const FrameBufferProperties& props)
        : props_(props)
    {
        for (const auto& tex_props : props.attachments) {
            if (internals::is_depth_format(tex_props.format))
                depth_attachment_ = tex_props;
            else
                colour_attachments_.push_back(tex_props);
        }

        invalidate();
    }

    FrameBuffer::~FrameBuffer()
    {
        GL_Call(glDeleteFramebuffers(1, &renderer_id_));
        GL_Call(glDeleteTextures(colour_attachment_ids_.size(), colour_attachment_ids_.data()));
        GL_Call(glDeleteTextures(1, &depth_attachment_id_));
    }

    void FrameBuffer::invalidate()
    {
        if (renderer_id_) {
            GL_Call(glDeleteFramebuffers(1, &renderer_id_));
            GL_Call(glDeleteTextures(colour_attachment_ids_.size(), colour_attachment_ids_.data()));
            GL_Call(glDeleteTextures(1, &depth_attachment_id_));

            colour_attachment_ids_.clear();
            depth_attachment_id_ = 0;
        }

        GL_Call(glGenFramebuffers(1, &renderer_id_));
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, renderer_id_));

        if (!colour_attachments_.empty()) {
            colour_attachment_ids_.resize(colour_attachments_.size());
            GL_Call(glGenTextures(colour_attachment_ids_.size(), colour_attachment_ids_.data()));

            for (usize i = 0; i < colour_attachment_ids_.size(); ++i) {
                glBindTexture(GL_TEXTURE_2D, colour_attachment_ids_[i]);
                internals::attach_colour_texture(colour_attachment_ids_[i], colour_attachments_[i], props_.width,
                                                 props_.height, i);
            }
        }

        if (depth_attachment_.format != TextureFormat::None) {
            GL_Call(glGenTextures(1, &depth_attachment_id_));
            GL_Call(glBindTexture(GL_TEXTURE_2D, depth_attachment_id_));

            switch (depth_attachment_.format) {
                case TextureFormat::Depth24Stencil8:
                    internals::attach_depth_texture(depth_attachment_id_, depth_attachment_, props_.width,
                                                    props_.height);
                    break;
                default: break;
            }
        }

        if (!colour_attachments_.empty()) {
            GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, renderer_id_));
            cxassert(colour_attachments_.size() < MAX_COLOUR_ATTACHMENT_COUNT,
                     "Cannot have more than {} colour attachments.", MAX_COLOUR_ATTACHMENT_COUNT);
            cxassert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                     "Error: Framebuffer is not complete.");

            GLenum buffers[MAX_COLOUR_ATTACHMENT_COUNT];
            for (int i = 0; i < MAX_COLOUR_ATTACHMENT_COUNT; ++i)
                buffers[i] = GL_COLOR_ATTACHMENT0 + i;

            GL_Call(glDrawBuffers(MAX_COLOUR_ATTACHMENT_COUNT, buffers));
        } else {
            glDrawBuffer(GL_NONE);
        }
    }

    void FrameBuffer::bind()
    {
        GL_Call(glBindFramebuffer(GL_FRAMEBUFFER, renderer_id_));
        GL_Call(glViewport(0, 0, props_.width, props_.height));
    }

    void FrameBuffer::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::resize(const u32 width, const u32 height)
    {
        if (width != 0 && height != 0 && width < MAX_FRAME_BUFFER_SIZE && height < MAX_FRAME_BUFFER_SIZE) {
            props_.width  = width;
            props_.height = height;
            invalidate();
        }
    }

    int FrameBuffer::read_pixel(const u32 index, const i32 x, const i32 y)
    {
        cxassert(index < colour_attachment_ids_.size(), "Index outside bounds of attachments.");
        GL_Call(glReadBuffer(GL_COLOR_ATTACHMENT0 + index));
        int pixel_data;
        GL_Call(glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel_data));
        return pixel_data;
    }

    void FrameBuffer::set_currently_bound_texture_properties(const TextureWrapMode   wrap_mode,
                                                             const TextureFilterMode filter_mode)
    {
        switch (wrap_mode) {
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

        switch (filter_mode) {
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

    GLenum FrameBuffer::determine_format_type(const TextureFormat format)
    {
        switch (format) {
            case TextureFormat::Depth24Stencil8: return GL_UNSIGNED_INT_24_8;
            case TextureFormat::RGB32F: return GL_FLOAT;
            case TextureFormat::RGBA8: return GL_UNSIGNED_BYTE;
            default: cxassert(false, "Bad texture format.");
        }
        return (GLenum)0;
    }
} // namespace codex::opengl
