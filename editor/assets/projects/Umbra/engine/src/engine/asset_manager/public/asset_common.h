#pragma once

#include <engine/core/public/serializer.h>
#include <engine/filesystem/public/file_handle.h>
#include <engine/memory/public/memory.h>

#define CX_ASSET(type)                                                                                                 \
    static constexpr std::string_view __cx_intrinsics_type_name = #type;                                               \
                                                                                                                       \
public:                                                                                                                \
    [[nodiscard]] static constexpr std::string_view ktype_name() noexcept                                              \
    {                                                                                                                  \
        return __cx_intrinsics_type_name;                                                                              \
    }                                                                                                                  \
                                                                                                                       \
public:                                                                                                                \
    [[nodiscard]] std::string_view type_name() const noexcept override                                                 \
    {                                                                                                                  \
        return __cx_intrinsics_type_name;                                                                              \
    }                                                                                                                  \
                                                                                                                       \
private:

#define CX_ASSET_IMPORT_SETTINGS(type)                                                                                 \
    [[nodiscard]] Box<IAssetImportSettings> clone() const noexcept override                                            \
    {                                                                                                                  \
        return Box<type>::make(*this);                                                                                 \
    }

namespace codex {
    struct IAssetImportSettings : public ISerializable
    {
        [[nodiscard]] virtual Box<IAssetImportSettings> clone() const noexcept = 0;
    };

    class IAsset
    {
    public:
        [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
    };

    class IAssetLoader
    {
    public:
        [[nodiscard]] virtual Shared<void>              load_asset(Shared<fs::FileHandle>      fh,
                                                                   const IAssetImportSettings* params = nullptr) const noexcept = 0;
        [[nodiscard]] virtual std::type_index           asset_type_id() const noexcept   = 0;
        [[nodiscard]] virtual std::string_view          asset_type_name() const noexcept = 0;
        [[nodiscard]] virtual Box<IAssetImportSettings> default_import_settings() const noexcept { return nullptr; };
    };

    template <typename T>
    concept AssetType = requires(T& obj) {
        { std::derived_from<T, IAsset> };
        { obj.type_name() } -> std::convertible_to<std::string_view>;
    };

    template <typename T>
    concept ImportSettingsType = requires {
        { std::derived_from<T, IAssetImportSettings> };
    };

    template <AssetType TAsset, ImportSettingsType TAssetImportSettings>
    class AssetLoaderBase : public IAssetLoader
    {
    public:
        [[nodiscard]] Shared<void> load_asset(Shared<fs::FileHandle>      fh,
                                              const IAssetImportSettings* params = nullptr) const noexcept override
        {
            if (params)
                return load(fh, *static_cast<const TAssetImportSettings*>(params));
            return load(fh, TAssetImportSettings{});
        }
        [[nodiscard]] virtual Shared<TAsset> load(Shared<fs::FileHandle>      fh,
                                                  const TAssetImportSettings& params) const noexcept = 0;
        [[nodiscard]] std::type_index        asset_type_id() const noexcept { return typeid(TAsset); }
        [[nodiscard]] std::string_view       asset_type_name() const noexcept override { return TAsset::ktype_name(); }
        [[nodiscard]] Box<IAssetImportSettings> default_import_settings() const noexcept override
        {
            return Box<TAssetImportSettings>::make().template as<TAssetImportSettings>();
        }
    };

    template <AssetType TAsset>
    class AssetLoaderBase<TAsset, void> : public IAssetLoader
    {
    public:
        [[nodiscard]] Shared<void> load_asset(Shared<fs::FileHandle> fh, const IAssetImportSettings*) const noexcept
        {
            return load(fh).template as<void>();
        }
        [[nodiscard]] virtual Shared<TAsset> load(Shared<fs::FileHandle> fh) const noexcept = 0;
        [[nodiscard]] std::type_index        asset_type_id() const noexcept override { return typeid(TAsset); }
        [[nodiscard]] std::string_view       asset_type_name() const noexcept override { return TAsset::ktype_name(); }
    };

    struct AssetPath : public ISerializable
    {
    public:
        AssetPath() noexcept
            : path_{}
            , uuid_{}
            , hash_{}
        {
        }
        AssetPath(const std::string_view path, const UUID uuid = UUID{}) noexcept
            : path_{ path }
            , uuid_{ uuid }
            , hash_{ std::hash<std::string>{}(path_) }
        {
        }

    public:
        [[nodiscard]] const UUID&        uuid() const noexcept { return uuid_; }
        [[nodiscard]] const std::string& path() const noexcept { return path_; }
        [[nodiscard]] usize              hash() const noexcept { return hash_; }

    public:
        [[nodiscard]] bool operator==(const AssetPath& other) const noexcept
        {
            return uuid_ == other.uuid_ && hash_ == other.hash_;
        }

    public:
        void serialize(ISerializationNode& node) const noexcept final override
        {
            node.write("path", path_);
            uuid_.serialize(node);
        }
        void deserialize(const ISerializationNode& node) noexcept final override
        {
            node.read("path", path_);
            uuid_.deserialize(node);
        }

    private:
        std::string path_;
        UUID        uuid_;
        usize       hash_;
    };

    template <typename T>
    concept AssetLoader = requires {
        { std::derived_from<T, IAssetLoader> };
    };

    template <AssetType TAsset>
    class Asset
    {
    public:
        using value_type = TAsset;

    public:
        Asset() noexcept = default;
        Asset(std::nullopt_t) noexcept {}
        Asset(AssetPath path, Shared<TAsset> asset) noexcept
            : path_{ std::move(path) }
            , asset_{ std::move(asset) }
        {
        }

        Asset(const Asset&) noexcept            = default;
        Asset(Asset&&) noexcept                 = default;
        Asset& operator=(const Asset&) noexcept = default;
        Asset& operator=(Asset&&) noexcept      = default;
        Asset& operator=(std::nullopt_t) noexcept
        {
            reset();
            return *this;
        }

    public:
        [[nodiscard]] TAsset&       operator*() noexcept { return *asset_; }
        [[nodiscard]] const TAsset& operator*() const noexcept { return *asset_; }
        [[nodiscard]] TAsset*       operator->() noexcept { return asset_.get(); }
        [[nodiscard]] const TAsset* operator->() const noexcept { return asset_.get(); }

        [[nodiscard]] TAsset& value() noexcept
        {
            CX_ASSERT(!asset_, "Attempted to dereference an invalid asset handle.");
            return *asset_;
        }
        [[nodiscard]] const TAsset& value() const noexcept
        {
            CX_ASSERT(!asset_, "Attempted to dereference an invalid asset handle.");
            return *asset_;
        }

        [[nodiscard]] const AssetPath& path() const noexcept { return path_; }
        [[nodiscard]] UUID             uuid() const noexcept { return path_.uuid(); }

        [[nodiscard]] bool     valid() const noexcept { return asset_ != nullptr; }
        [[nodiscard]] explicit operator bool() const noexcept { return valid(); }

        [[nodiscard]] Shared<TAsset> shared() const noexcept { return asset_; }

        Shared<TAsset> release() noexcept
        {
            path_ = {};
            return std::move(asset_);
        }
        void reset() noexcept
        {
            asset_.reset();
            path_ = {};
        }

    private:
        AssetPath      path_;
        Shared<TAsset> asset_;
    };
} // namespace codex

namespace std {
    template <>
    struct hash<codex::AssetPath>
    {
        [[nodiscard]] std::size_t operator()(const codex::AssetPath& path) const noexcept
        {
            return std::hash<codex::UUID>{}(path.uuid()) ^ std::hash<std::string>{}(path.path());
        }
    };
} // namespace std

namespace fmt {
    template <>
    struct formatter<codex::AssetPath> : formatter<std::string_view>
    {
        auto format(const codex::AssetPath& path, format_context& ctx) const
        {
            return format_to(ctx.out(), "{{{}}}@{}", path.uuid().to_string(), path.path());
        }
    };
} // namespace fmt
