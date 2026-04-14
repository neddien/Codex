#pragma once

#include <engine/filesystem/public/file_handle.h>
#include <engine/memory/public/memory.h>

#define CX_ASSET(type)                                                                                                 \
    [[nodiscard]] std::string_view type_name() const noexcept override                                                 \
    {                                                                                                                  \
        return #type;                                                                                                  \
    }

namespace codex {
    class IAssetLoader
    {
    public:
        virtual ~IAssetLoader() noexcept                                                                 = default;
        [[nodiscard]] virtual Shared<void> load(Shared<fs::FileHandle> fh, const void* params = nullptr) = 0;
        [[nodiscard]] virtual Shared<void> load_serialized(Shared<fs::FileHandle>    fh,
                                                           const ISerializationNode& node)               = 0;
    };

    template <typename T>
    concept AssetLoader = std::derived_from<T, IAssetLoader> && requires { typename T::asset_type; };

    template <typename T>
    concept ParameterizedAssetLoader = AssetLoader<T> && requires {
        typename T::import_settings;
        requires !std::same_as<typename T::import_settings, void>;
    };

    template <typename T>
    concept RegisteredAsset = requires {
        typename T::loader_type;
        requires AssetLoader<typename T::loader_type>;
        requires std::same_as<typename T::loader_type::asset_type, T>;
    };

    template <typename TAsset, typename TImportSettings>
    class AssetLoaderBase : public IAssetLoader
    {
    public:
        using asset_type      = TAsset;
        using import_settings = TImportSettings;

    public:
        virtual ~AssetLoaderBase() noexcept = default;

    public:
        [[nodiscard]] Shared<void> load(Shared<fs::FileHandle> fh, const void* params) final override
        {
            return load_asset(std::move(fh), *static_cast<const TImportSettings*>(params)).template as<void>();
        }
        [[nodiscard]] Shared<void> load_serialized(Shared<fs::FileHandle>    fh,
                                                   const ISerializationNode& node) final override
        {
            return load_serialized_asset(std::move(fh), node).template as<void>();
        }

        [[nodiscard]] virtual Shared<TAsset>   load_asset(Shared<fs::FileHandle> fh, const TImportSettings& params) = 0;
        [[nodiscard]] virtual Shared<TAsset>   load_serialized_asset(Shared<fs::FileHandle>    fh,
                                                                     const ISerializationNode& node)                = 0;
        [[nodiscard]] virtual std::string_view type_name() const noexcept                                           = 0;
    };

    // Specialization for loaders that don't take parameters.
    template <typename TAsset>
    class AssetLoaderBase<TAsset, void> : public IAssetLoader
    {
    public:
        using asset_type = TAsset;

        Shared<void> load(Shared<fs::FileHandle> fh, const void* /*params*/) final override
        {
            return load_asset(std::move(fh)).template as<void>();
        }

        virtual Shared<TAsset> load_asset(Shared<fs::FileHandle> fh) = 0;
    };

    struct AssetPath
    {
        UUID        uuid;
        std::string path;
    };

    template <RegisteredAsset TAsset>
    class Asset
    {
    public:
        Asset() = default;
        constexpr Asset(std::nullptr_t) noexcept {}
        Asset(std::string_view path, Shared<TAsset> asset)
            : path_{ .path = std::string(path) }
            , asset_(std::move(asset))
        {
        }

    public:
        [[nodiscard]] TAsset*       get() noexcept { return asset_.get(); }
        [[nodiscard]] const TAsset* get() const noexcept { return asset_.get(); }
        [[nodiscard]] TAsset*       operator->() noexcept { return asset_.get(); }
        [[nodiscard]] const TAsset* operator->() const noexcept { return asset_.get(); }
        [[nodiscard]] TAsset&       operator*() noexcept { return *asset_; }
        [[nodiscard]] const TAsset& operator*() const noexcept { return *asset_; }
        explicit                    operator bool() const noexcept { return static_cast<bool>(asset_); }

        [[nodiscard]] const std::string& path() const noexcept { return path_.path; }
        [[nodiscard]] const UUID&        uuid() const noexcept { return path_.uuid; }

        [[nodiscard]] const Shared<TAsset>& shared() const noexcept { return asset_; }

    private:
        AssetPath      path_;
        Shared<TAsset> asset_;
    };
} // namespace codex
