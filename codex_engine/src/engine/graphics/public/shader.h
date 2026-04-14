#pragma once

#include <engine/core/public/exception.h>
#include <engine/core/public/i_resource.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/shader.h>

namespace codex::gfx {
    CX_CUSTOM_EXCEPTION(ShaderException, "Bad shader.")
    CX_CUSTOM_EXCEPTION(ShaderNotFoundException, "Shader file was not found.")

    class CODEX_API Shader : public IResource
    {
        friend class ResourceHandler;

    public:
        Shader(std::filesystem::path file_path, const std::string_view version = "330 core")
        {
            path_       = file_path;
            raw_shader_ = Box<opengl::Shader>::make(std::move(file_path), version);
        }
        Shader(const Shader&)                = delete;
        Shader& operator=(const Shader&)     = delete;
        Shader(Shader&&) noexcept            = default;
        Shader& operator=(Shader&&) noexcept = default;

    public:
        [[nodiscard]] inline std::filesystem::path file_path() const noexcept { return raw_shader_->file_path(); }
        [[nodiscard]] inline std::string_view      version() const noexcept { return raw_shader_->version(); }

    public:
        inline void bind() const { raw_shader_->bind(); }
        inline void unbind() const { raw_shader_->unbind(); }
        inline void set_uniform_1i(const char* name, const i32 value) { raw_shader_->set_uniform_1i(name, value); }
        inline void set_uniform_1i_arr(const char* name, const u32 count, const i32* value)
        {
            raw_shader_->set_uniform_1i_arr(name, count, value);
        }
        inline void set_uniform_1f(const char* name, const f32 value) { raw_shader_->set_uniform_1f(name, value); }
        inline void set_uniform_2f(const char* name, const f32 a0, const f32 a1)
        {
            raw_shader_->set_uniform_2f(name, a0, a1);
        }
        inline void set_uniform_4f(const char* name, const f32 a0, const f32 a1, const f32 a2, const f32 a3)
        {
            raw_shader_->set_uniform_4f(name, a0, a1, a2, a3);
        }
        inline void set_uniform_3f(const char* name, const f32 a0, const f32 a1, const f32 a2)
        {
            raw_shader_->set_uniform_3f(name, a0, a1, a2);
        }
        inline void set_uniform_mat4f(const char* name, const glm::mat4& matrix)
        {
            raw_shader_->set_uniform_mat4f(name, matrix);
        }
        inline void set_uniform_mat4f(const char* name, const f32* matrix)
        {
            raw_shader_->set_uniform_mat4f(name, matrix);
        }
        inline bool compile_shader(
            const std::initializer_list<std::pair<std::string_view, std::string_view>> compile_definitions = {})
        {
            return raw_shader_->compile_shader(compile_definitions);
        }

    public:
        void serialize(ISerializationNode& node) const override
        {
            node.write("id", id());
            node.write("file_path", raw_shader_->file_path().generic_string());
            node.write("version", raw_shader_->version());
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto path    = std::string{};
            auto version = std::string{};

            node.read("id", id_);
            node.read("file_path", path);
            node.read("version", version);

            raw_shader_ = Box<opengl::Shader>::make(std::move(path), version);
        }

    private:
        Box<opengl::Shader> raw_shader_;
    };
} // namespace codex::gfx
