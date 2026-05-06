#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/core/public/exception.h>
#include <engine/memory/public/memory.h>
#include <platform/open_gl/shader.h>

namespace codex::gfx {
    CX_CUSTOM_EXCEPTION(ShaderException, "Bad shader.")
    CX_CUSTOM_EXCEPTION(ShaderNotFoundException, "Shader file was not found.")

    class CODEX_API Shader : public IAsset
    {
        CX_ASSET(Shader)

    public:
        struct ImportSettings : public IAssetImportSettings
        {
            CX_ASSET_IMPORT_SETTINGS(ImportSettings)

            ImportSettings() : version{ "330 core" } {}

            std::string version;

            void serialize(ISerializationNode& node) const override { node.write("version", version); }
            void deserialize(const ISerializationNode& node) override { node.read("version", version); }
        };

    public:
        Shader() = default;
        explicit Shader(std::string source, const ImportSettings& settings = {})
            : raw_shader_{ Box<opengl::Shader>::make(std::move(source), settings.version) }
        {
        }

        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&)                 = default;
        Shader& operator=(Shader&&)      = default;

    public:
        [[nodiscard]] inline std::string_view version() const noexcept { return raw_shader_->version(); }
        [[nodiscard]] inline explicit operator bool() const noexcept { return (bool)raw_shader_; }

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

    private:
        Box<opengl::Shader> raw_shader_ = nullptr;
    };

    class ShaderLoader : public AssetLoaderBase<Shader, Shader::ImportSettings>
    {
        [[nodiscard]] Shared<Shader> load(Shared<fs::FileHandle>        fh,
                                          const Shader::ImportSettings& settings) const noexcept override
        {
            std::string source(fh->size(), '\0');
            fh->read(source.data(), fh->size());
            return Shared<Shader>::make(std::move(source), settings);
        }
    };
} // namespace codex::gfx
