#pragma once

#include <engine/core/public/exception.h>
#include <engine/core/public/i_resource.h>
#include <engine/memory/public/memory.h>

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
        void load();

    public:
        u8*       raw() noexcept { return raw_data_; }
        const u8* raw() const noexcept { return raw_data_; }
        i32       width() const noexcept { return width_; }
        i32       height() const noexcept { return height_; }
        i32       channels() const noexcept { return channels_; }

    public:
        void friend std::swap(Image2D& lhv, Image2D& rhv) noexcept;

    public:
        void serialize(ISerializationNode& node) const override
        {
            node.write("id", id());
            node.write("file_path", path_.generic_string());
        }
        void deserialize(const ISerializationNode& node) override
        {
            auto path = std::string{};

            node.read("id", id_);
            node.read("path", path);

            path_ = path;

            load();
        }

    private:
        u8* raw_data_ = nullptr;
        i32 width_    = 0;
        i32 height_   = 0;
        i32 channels_ = 0;
    };
} // namespace codex::gfx
