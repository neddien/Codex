#pragma once

#include <sdafx.h>

#include <Engine/Core/Public/Exception.h>
#include <Engine/Core/Public/IResource.h>
#include <Engine/Memory/Public/Memory.h>

namespace codex::gfx {
    // Forward declerations.
    class Image2D;
} // namespace codex::gfx

namespace std {
    void swap(codex::gfx::Image2D& lhv, codex::gfx::Image2D& rhv) noexcept;
} // namespace std

namespace codex::gfx {
    CX_CUSTOM_EXCEPTION(ImageLoadException, "Failed to load the image.")

    // Wrapper over stb_image.
    class CODEX_API Image2D : public IResource
    {
    private:
        u8*                   m_RawData  = nullptr;
        i32                   m_Width    = 0;
        i32                   m_Height   = 0;
        i32                   m_Channels = 0;
        std::filesystem::path m_Path{};

    public:
        Image2D() = default;
        explicit Image2D(std::filesystem::path file);
        Image2D(const Image2D& other);
        Image2D(Image2D&& other) noexcept;
        ~Image2D();

    public:
        constexpr operator bool() const noexcept;
        Image2D&  operator=(const Image2D& other);
        Image2D&  operator=(Image2D&& other) noexcept;

    private:
        void Load();

    public:
        u8*                          GetRaw() noexcept { return m_RawData; }
        const u8*                    GetRaw() const noexcept { return m_RawData; }
        i32                          GetWidth() const noexcept { return m_Width; }
        i32                          GetHeight() const noexcept { return m_Height; }
        i32                          GetChannels() const noexcept { return m_Channels; }
        const std::filesystem::path& GetPath() const noexcept { return m_Path; }

    public:
        void friend std::swap(Image2D& lhv, Image2D& rhv) noexcept;

    public:
        void Serialize(ISerializationNode& node) const override
        {
            node.Write("id", GetId());
            node.Write("file_path", m_Path);
        }
        void Deserialize(const ISerializationNode& node) override
        {
            auto        path  = std::filesystem::path{};

            node.Read("id", m_Id);
            node.Read("path", path);

            Load();
        }
    };
} // namespace codex::gfx
