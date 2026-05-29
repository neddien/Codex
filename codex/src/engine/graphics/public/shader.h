#pragma once

#include <engine/asset_manager/public/asset_common.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/core/public/exception.h>
#include <engine/memory/public/memory.h>

namespace codex::opengl {
    class Shader;
}

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

            ImportSettings()
                : version{ "330 core" }
            {
            }

            std::string version;

            void serialize(ISerializationNode& node) const override;
            void deserialize(const ISerializationNode& node) override;
        };

    public:
        Shader() = default;
        explicit Shader(std::string source, const ImportSettings& settings = {});
        ~Shader();

        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader&&) noexcept;

    public:
        [[nodiscard]] std::string_view version() const noexcept;
        [[nodiscard]] explicit         operator bool() const noexcept;

    public:
        void bind() const;
        void unbind() const;
        void set_uniform_1i(const char* name, i32 value);
        void set_uniform_1i_arr(const char* name, u32 count, const i32* value);
        void set_uniform_1f(const char* name, f32 value);
        void set_uniform_2f(const char* name, f32 a0, f32 a1);
        void set_uniform_3f(const char* name, f32 a0, f32 a1, f32 a2);
        void set_uniform_4f(const char* name, f32 a0, f32 a1, f32 a2, f32 a3);
        void set_uniform_mat4f(const char* name, const glm::mat4& matrix);
        void set_uniform_mat4f(const char* name, const f32* matrix);
        bool compile_shader(
            std::initializer_list<std::pair<std::string_view, std::string_view>> compile_definitions = {});

    private:
        Box<opengl::Shader> raw_shader_;
    };

    class ShaderLoader : public AssetLoaderBase<Shader, Shader::ImportSettings>
    {
        [[nodiscard]] Shared<Shader> load(Shared<fs::FileHandle>        fh,
                                          const Shader::ImportSettings& settings) const noexcept override;
    };
} // namespace codex::gfx
