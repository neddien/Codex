#pragma once

#include "Constants.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace codex::opengl {
    class CODEX_API Shader
    {
    private:
        uint32_t                                 m_RendererId;
        std::filesystem::path                    m_FilePath;
        std::string                              m_VertexSource;
        std::string                              m_FragmentSource;
        uint32_t                                 m_VertexShaderId;
        uint32_t                                 m_FragmentShaderId;
        std::unordered_map<std::string, int32_t> m_UniformLocations;
        std::string                              m_Version;

    public:
        Shader(std::filesystem::path filePath, const std::string_view version = "330 core");
        ~Shader();

    public:
        [[nodiscard]] std::filesystem::path GetFilePath() const noexcept { return m_FilePath; }
        [[nodiscard]] std::string_view      GetVersion() const noexcept { return m_Version; }

    public:
        void Bind() const;
        void Unbind() const;
        void SetUniform1i(const char* name, const int32_t value);
        void SetUniform1iArr(const char* name, const uint32_t count, const int32_t* value);
        void SetUniform1f(const char* name, const float value);
        void SetUniform2f(const char* name, const float a0, const float a1);
        void SetUniform4f(const char* name, const float a0, const float a1, const float a2, const float a3);
        void SetUniform3f(const char* name, const float a0, const float a1, const float a2);
        void SetUniformMat4f(const char* name, const glm::mat4& matrix);
        void SetUniformMat4f(const char* name, const float* matrix);
        bool CompileShader(
            const std::initializer_list<std::pair<std::string_view, std::string_view>> compileDefinitions = {});

    private:
        void    ParseShaderFile();
        int32_t GetUniformLocation(const char* name);
    };
} // namespace codex::opengl
