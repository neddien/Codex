#pragma once

#include <engine/algorithm/public/dense_vector.h>
#include <engine/asset_manager/public/asset_manager.h>
#include <engine/core/public/common_third_party_libs.h>
#include <engine/memory/public/memory.h>
#include <engine/native_behaviour/public/native_behaviour.h>

namespace codex {
    class Scene;

    namespace sys {
        class DLib;
    }

    // TODO: Add  thread safety
    class CODEX_API NBMan : public System<NBMan>, public Loggable<"NativeBehaviourManager">
    {
    public:
        using FactoryFn = std::function<Box<NativeBehaviour>()>;

    public:
        struct BHRecord
        {
            std::string type_name;
            usize       type_hash;
            FactoryFn   factory;
        };

        // While NBMan does implement the System<T> trait meaning its ctors are already defined
        // but we're gonna need to define them here explicitly to prevent the compiler from
        // synthesizes them inline and cuasing compile errors in the public domain because of DLib.
        // More specifically editor projects fail to compile.
    public:
        NBMan() noexcept;
        ~NBMan() noexcept;
        NBMan(NBMan&&) noexcept            = delete;
        NBMan& operator=(NBMan&&) noexcept = delete;
        NBMan(const NBMan&)                = delete;
        NBMan& operator=(const NBMan&)     = delete;

    public:
        static void init();
        static void dispose() noexcept;

    public:
        template <typename T>
        static void register_type(const std::string_view type_name)
        {
            auto&    self = get();
            BHRecord record{
                .type_name = std::string{ type_name },
                .type_hash = util::crypto::fnv1a(type_name),
                .factory   = [] { return Box<T>::make(); },
            };
            self.types_[record.type_hash] = std::move(record);
        }

    public:
        static void                                   load(const std::filesystem::path path, Scene& scene);
        static void                                   unload(const bool save_to_pending = true);
        [[nodiscard]] static bool                     is_type_registered(const std::string_view type_name);
        [[nodiscard]] static std::vector<std::string> registered_types() noexcept;
        [[nodiscard]] static bool                     instance_loaded() noexcept;
        [[nodiscard]] static const BHRecord*          type_record(const std::string_view type) noexcept;

    private:
        Box<sys::DLib>                       nb_instance_;
        Scene*                               scene_ = nullptr;
        absl::flat_hash_map<usize, BHRecord> types_;
    };

    // Null-loader, meaning just register it into the Asset Registry but don't treat it as an Asset.
    // This is so that it will show up in the Content Browser inside the Editor.
    class NBScriptSourceNullLoader : public NullAssetLoader<"CXXSource">
    {
    };

    class NBScriptHeaderNullLoader : public NullAssetLoader<"CXXHeader">
    {
    };
} // namespace codex
