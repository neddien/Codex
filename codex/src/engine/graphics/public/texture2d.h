#pragma once

#include <engine/asset_manager/public/asset_manager.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/graphics/public/texture_properties.h>
#include <engine/memory/public/memory.h>

namespace codex::opengl {
    class Texture;
}

namespace codex::gfx {
    class CODEX_API Texture2D : public IAsset
    {
        CX_ASSET(Texture2D)

    public:
        struct ImportSettings : public IAssetImportSettings
        {
            CX_ASSET_IMPORT_SETTINGS(ImportSettings)

        public:
            TextureProperties props;

        public:
            ImportSettings() noexcept = default;
            explicit ImportSettings(const TextureProperties& props) noexcept
                : props{ props }
            {
            }

        public:
            void serialize(ISerializationNode& node) const override;
            void deserialize(const ISerializationNode& node) override;
        };

    public:
        Texture2D() = default;
        explicit Texture2D(const u8* buf, usize len, const ImportSettings& import_settings = {});
        Texture2D(const Texture2D& other);
        Texture2D(Texture2D&& other) noexcept;
        Texture2D& operator=(const Texture2D& other);
        Texture2D& operator=(Texture2D&& other) noexcept;
        ~Texture2D();

    public:
        [[nodiscard]] explicit                 operator bool() const noexcept;
        [[nodiscard]] u32                      gl_id() const noexcept;
        [[nodiscard]] u32                      slot() const;
        [[nodiscard]] i32                      width() const;
        [[nodiscard]] i32                      height() const;
        [[nodiscard]] const TextureProperties& properties() const noexcept;

    public:
        Texture2D& swap(Texture2D& other) noexcept;
        void       bind(u32 slot = 0);
        void       unbind() const;
        void       create(const u8* buf, usize len, const ImportSettings& import_settings = {});

    private:
        const u8*            buf_ = nullptr;
        usize                len_ = 0;
        TextureProperties    props_;
        Box<opengl::Texture> raw_texture_;
    };

    class Texture2DLoader : public AssetLoaderBase<Texture2D, Texture2D::ImportSettings>
    {
        [[nodiscard]] Shared<Texture2D> load(Shared<fs::FileHandle>           fh,
                                             const Texture2D::ImportSettings& params) const noexcept override;
    };
} // namespace codex::gfx
