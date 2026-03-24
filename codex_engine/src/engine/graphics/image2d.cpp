#include "public/image2d.h"

#include <stb_image.h>

namespace codex::gfx {
    namespace fs = std::filesystem;

    Image2D::Image2D(fs::path file)
    {
        path_ = std::move(file);
        load();
    }

    Image2D::Image2D(const Image2D& other)
        : width_(other.width_)
        , height_(other.height_)
        , channels_(other.channels_)
    {
        path_ = other.path_;

        const auto total_size = width_ * height_ * channels_;

        raw_data_ = new u8[total_size];
        std::memcpy(raw_data_, other.raw_data_, total_size);
    }

    Image2D::Image2D(Image2D&& other) noexcept
    {
        std::swap(*this, other);
    }

    Image2D::~Image2D()
    {
        stbi_image_free(raw_data_);
        raw_data_ = nullptr;
        width_    = 0;
        height_   = 0;
        channels_ = 0;
        path_     = fs::path{};
    }

    void Image2D::load()
    {
        const auto& str_path = path_.string();

        if (!fs::exists(path_))
            throw FileNotFoundException("Image was not found. Path: {}", str_path);

        if (!stbi_info(str_path.c_str(), &width_, &height_, &channels_))
            throw ImageLoadException("Could not query information about the supplied image. Path: {}", str_path);

        raw_data_ = stbi_load(path_.string().c_str(), &width_, &height_, &channels_, STBI_rgb_alpha);
    }

    constexpr Image2D::operator bool() const noexcept
    {
        return raw_data_;
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
        std::swap(lhv.id_, rhv.id_);
        std::swap(lhv.raw_data_, rhv.raw_data_);
        std::swap(lhv.width_, rhv.width_);
        std::swap(lhv.height_, rhv.height_);
        std::swap(lhv.channels_, rhv.channels_);
        std::swap(lhv.path_, rhv.path_);
    }
} // namespace std
