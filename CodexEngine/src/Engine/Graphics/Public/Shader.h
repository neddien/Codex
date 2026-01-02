#ifndef CODEX_RENDERER_SHADER_H
#define CODEX_RENDERER_SHADER_H

#include <sdafx.h>

#include <Engine/Core/Public/Exception.h>
#include <Engine/Core/Public/IResource.h>
#include <Engine/Memory/Public/Memory.h>
#include <Platform/OpenGL/Shader.h>

namespace codex::gfx {
    CX_CUSTOM_EXCEPTION(ShaderException, "Bad shader.")
    CX_CUSTOM_EXCEPTION(ShaderNotFoundException, "Shader file was not found.")

    class CODEX_API Shader : public IResource
    {
        friend class ResourceHandler;

    private:
        mem::Box<opengl::Shader> m_RawShader;

    public:
        Shader(std::filesystem::path filePath, const std::string_view version = "330 core")
        {
            m_Path      = filePath;
            m_RawShader = mem::Box<opengl::Shader>::New(std::move(filePath), version);
        }

    public:
        inline void Bind() const { m_RawShader->Bind(); }
        inline void Unbind() const { m_RawShader->Unbind(); }
        inline void SetUniform1i(const char* name, const i32 value) { m_RawShader->SetUniform1i(name, value); }
        inline void SetUniform1iArr(const char* name, const u32 count, const i32* value)
        {
            m_RawShader->SetUniform1iArr(name, count, value);
        };
        inline void SetUniform1f(const char* name, const f32 value) { m_RawShader->SetUniform1f(name, value); }
        inline void SetUniform2f(const char* name, const f32 a0, const f32 a1)
        {
            m_RawShader->SetUniform2f(name, a0, a1);
        }
        inline void SetUniform4f(const char* name, const f32 a0, const f32 a1, const f32 a2, const f32 a3)
        {
            m_RawShader->SetUniform4f(name, a0, a1, a2, a3);
        }
        inline void SetUniform3f(const char* name, const f32 a0, const f32 a1, const f32 a2)
        {
            m_RawShader->SetUniform3f(name, a0, a1, a2);
        }
        inline void SetUniformMat4f(const char* name, const glm::mat4& matrix)
        {
            m_RawShader->SetUniformMat4f(name, matrix);
        }
        inline void SetUniformMat4f(const char* name, const f32* matrix) { m_RawShader->SetUniformMat4f(name, matrix); }
        inline bool CompileShader(
            const std::initializer_list<std::pair<std::string_view, std::string_view>> compileDefinitions = {})
        {
            return m_RawShader->CompileShader(compileDefinitions);
        }

    public:
        void Serialize(ISerializationNode& node) const override
        {
            node.Write("id", GetId());
            node.Write("file_path", m_RawShader->GetFilePath().string());
            node.Write("version", m_RawShader->GetVersion());
        }
        void Deserialize(const ISerializationNode& node) override
        {
            auto path    = std::string{};
            auto version = std::string{};

            node.Read("id", m_Id);
            node.Read("file_path", path);
            node.Read("version", version);

            m_RawShader = mem::Box<opengl::Shader>::New(std::move(path), version);
        }
    };
} // namespace codex::gfx

#endif // CODEX_RENDERER_SHADER_H
