#ifndef CODEX_RENDERER_SHADER_H
#define CODEX_RENDERER_SHADER_H

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
        Shader(const Shader&)                = delete;
        Shader& operator=(const Shader&)     = delete;
        Shader(Shader&&) noexcept            = default;
        Shader& operator=(Shader&&) noexcept = default;

    public:
        Shader(std::filesystem::path filePath, const std::string_view version = "330 core")
        {
            path_       = filePath;
            raw_shader_ = mem::Box<opengl::Shader>::make(std::move(filePath), version);
        }

    public:
        inline void bind() const { raw_shader_->Bind(); }
        inline void unbind() const { raw_shader_->Unbind(); }
        inline void set_uniform_1i(const char* name, const i32 value) { raw_shader_->SetUniform1i(name, value); }
        inline void set_uniform_1i_arr(const char* name, const u32 count, const i32* value)
        {
            raw_shader_->SetUniform1iArr(name, count, value);
        };
        inline void set_uniform_1f(const char* name, const f32 value) { raw_shader_->SetUniform1f(name, value); }
        inline void set_uniform_2f(const char* name, const f32 a0, const f32 a1)
        {
            raw_shader_->SetUniform2f(name, a0, a1);
        }
        inline void set_uniform_4f(const char* name, const f32 a0, const f32 a1, const f32 a2, const f32 a3)
        {
            raw_shader_->SetUniform4f(name, a0, a1, a2, a3);
        }
        inline void set_uniform_3f(const char* name, const f32 a0, const f32 a1, const f32 a2)
        {
            raw_shader_->SetUniform3f(name, a0, a1, a2);
        }
        inline void set_uniform_mat4f(const char* name, const glm::mat4& matrix)
        {
            raw_shader_->SetUniformMat4f(name, matrix);
        }
        inline void set_uniform_mat4f(const char* name, const f32* matrix)
        {
            raw_shader_->SetUniformMat4f(name, matrix);
        }
        inline bool compile_shader(
            const std::initializer_list<std::pair<std::string_view, std::string_view>> compileDefinitions = {})
        {
            return raw_shader_->CompileShader(compileDefinitions);
        }

    public:
        void serialize(ISerializationNode& node) const override
        {
            node.write("id", id());
            node.write("file_path", raw_shader_->GetFilePath().generic_string());
            node.write("version", raw_shader_->GetVersion());
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto path    = std::string{};
            auto version = std::string{};

            node.read("id", id_);
            node.read("file_path", path);
            node.read("version", version);

            raw_shader_ = mem::Box<opengl::Shader>::make(std::move(path), version);
        }

    private:
        mem::Box<opengl::Shader> raw_shader_;
    };
} // namespace codex::gfx

#endif // CODEX_RENDERER_SHADER_H
