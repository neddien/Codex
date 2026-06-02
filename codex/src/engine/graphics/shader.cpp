#include "public/shader.h"

#include <platform/open_gl/shader.h>

namespace codex::gfx {
    Shader::Shader(std::string source, const ImportSettings& settings)
        : raw_shader_{ Box<opengl::Shader>::make(std::move(source), settings.version) }
    {
    }

    Shader::~Shader() = default;

    Shader::Shader(Shader&& other) noexcept
        : raw_shader_{ std::move(other.raw_shader_) }
    {
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        raw_shader_ = std::move(other.raw_shader_);
        return *this;
    }

    std::string_view Shader::version() const noexcept
    {
        return raw_shader_->version();
    }

    Shader::operator bool() const noexcept
    {
        return (bool)raw_shader_;
    }

    void Shader::bind() const
    {
        raw_shader_->bind();
    }

    void Shader::unbind() const
    {
        raw_shader_->unbind();
    }

    void Shader::set_uniform_1i(const char* name, const i32 value)
    {
        raw_shader_->set_uniform_1i(name, value);
    }

    void Shader::set_uniform_1i_arr(const char* name, const u32 count, const i32* value)
    {
        raw_shader_->set_uniform_1i_arr(name, count, value);
    }

    void Shader::set_uniform_1f(const char* name, const f32 value)
    {
        raw_shader_->set_uniform_1f(name, value);
    }

    void Shader::set_uniform_2f(const char* name, const f32 a0, const f32 a1)
    {
        raw_shader_->set_uniform_2f(name, a0, a1);
    }

    void Shader::set_uniform_3f(const char* name, const f32 a0, const f32 a1, const f32 a2)
    {
        raw_shader_->set_uniform_3f(name, a0, a1, a2);
    }

    void Shader::set_uniform_4f(const char* name, const f32 a0, const f32 a1, const f32 a2, const f32 a3)
    {
        raw_shader_->set_uniform_4f(name, a0, a1, a2, a3);
    }

    void Shader::set_uniform_mat4f(const char* name, const glm::mat4& matrix)
    {
        raw_shader_->set_uniform_mat4f(name, matrix);
    }

    void Shader::set_uniform_mat4f(const char* name, const f32* matrix)
    {
        raw_shader_->set_uniform_mat4f(name, matrix);
    }

    bool Shader::compile_shader(
        const std::initializer_list<std::pair<std::string_view, std::string_view>> compile_definitions)
    {
        return raw_shader_->compile_shader(compile_definitions);
    }

    void Shader::ImportSettings::archive(Archive& ar)
    {
        ar("version", version);
    }

    [[nodiscard]] Shared<Shader> ShaderLoader::load(Shared<fs::FileHandle>        fh,
                                                    const Shader::ImportSettings& settings) const noexcept
    {
        std::string source(fh->size(), '\0');
        fh->read(source.data(), fh->size());
        return Shared<Shader>::make(std::move(source), settings);
    }
} // namespace codex::gfx
