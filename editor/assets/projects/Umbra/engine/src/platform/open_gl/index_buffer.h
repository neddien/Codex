#pragma once

#include "constants.h"

namespace codex::opengl {
    // Forward declerations
    class Renderer;

    class CODEX_API IndexBuffer
    {
        friend class Renderer;

    public:
        IndexBuffer();
        ~IndexBuffer();

    public:
        [[nodiscard]] inline usize indices() const { return indices_; }

    public:
        void bind() const;
        void unbind() const;
        void set_buffer(const u32* data, const usize index_count,
                        const BufferUsage usage = BufferUsage::STATIC_DRAW);
        void set_buffer_sub_data(const u32* data, const usize index_count);

    private:
        u32   renderer_id_;
        usize indices_;
    };
} // namespace codex::opengl
