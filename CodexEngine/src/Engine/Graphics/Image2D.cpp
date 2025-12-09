#include "Public/Image2D.h"

#include <stb_image.h>

namespace codex::gfx {
    namespace fs = std::filesystem;

    Image2D::Image2D(fs::path file) : m_Path(std::move(file))
    {
        Load();
    }

    Image2D::Image2D(const Image2D& other)
        : m_Width(other.m_Width), m_Height(other.m_Height), m_Channels(other.m_Channels), m_Path(other.m_Path)
    {
        const auto total_size = m_Width * m_Height * m_Channels;

        m_RawData = new u8[total_size];
        std::memcpy(m_RawData, other.m_RawData, total_size);
    }

    Image2D::Image2D(Image2D&& other) noexcept
    {
        std::swap(*this, other);
    }

    Image2D::~Image2D()
    {
        stbi_image_free(m_RawData);
        m_RawData  = nullptr;
        m_Width    = 0;
        m_Height   = 0;
        m_Channels = 0;
        m_Path     = fs::path{};
    }

    void Image2D::Load()
    {
        const auto& str_path = m_Path.string();

        if (!fs::exists(m_Path))
            cx_throw(FileNotFoundException, "Image was not found. Path: {}", str_path);

        if (!stbi_info(str_path.c_str(), &m_Width, &m_Height, &m_Channels))
            cx_throw(ImageLoadException, "Could not query information about the supplied image. Path: {}", str_path);

        m_RawData = stbi_load(m_Path.string().c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);
    }

    constexpr Image2D::operator bool() const noexcept
    {
        return m_RawData;
    }

    Image2D& Image2D::operator=(const Image2D& other)
    {
        if (&other == this)
            return *this;

        Image2D copy(other);
        std::swap(copy, *this);
        return *this;
    }

    Image2D& Image2D::operator=(Image2D&& other) noexcept
    {
        if (&other == this)
            return *this;

        std::swap(*this, other);
        return *this;
    }
} // namespace codex::gfx

namespace std {
    void swap(codex::gfx::Image2D& lhv, codex::gfx::Image2D& rhv) noexcept
    {
        std::swap(lhv.m_Id, rhv.m_Id);
        std::swap(lhv.m_RawData, rhv.m_RawData);
        std::swap(lhv.m_Width, rhv.m_Width);
        std::swap(lhv.m_Height, rhv.m_Height);
        std::swap(lhv.m_Channels, rhv.m_Channels);
        std::swap(lhv.m_Path, rhv.m_Path);
    }
} // namespace std