#include <codex.h>
#include <engine/core/entry_point.h>

#include <launcher_settings.h>

using namespace codex;

class RuntimeLayer : public Layer, public Loggable<"RuntimeLayer">
{
public:
    RuntimeLayer(fs::VirtualFilesystem& vfs)
        : vfs_{ vfs }
    {
    }

public:
    void on_attach() override
    {
        const EngineProject& project = Engine::project();
        AssetManager::init(vfs_, project.assets_root);
        AssetManager::registry().from_manifest("/run/project/assets/__registry.manifest.bin");

        opengl::FrameBufferProperties props;
        props.attachments = {
            { .format = opengl::TextureFormat::RGBA8 },
            { .format = opengl::TextureFormat::RedInt32 },
            { .format = opengl::TextureFormat::Depth24Stencil8 },
        };

        // TODO: This is the scene render resolution so you should not hard code this.
        props.width  = 1920;
        props.height = 1080;
        framebuffer_ = Box<opengl::FrameBuffer>::make(props);

        gfx::Renderer::init(Engine::window().width(), Engine::window().height());
        gfx::BatchRenderer2D::init(vfs_, "/run/data/gl_shaders/batch_renderer2d_quad.glsl");
        gfx::Renderer::set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);

        Asset<Scene> scene = AssetManager::load<Scene>(project.boot_scene.uuid());
        if (!scene)
            throw CodexException("Failed to load: {}", project.boot_scene.uuid());

        active_scene_ = scene.as_shared();

        if (Engine::project().engine_properties.flags & EngineFlags::Audio) {
            const auto audio_dir = "/run/project/assets/audio/Build";
            if (vfs_.exists(audio_dir)) {
                for (
                    const auto& entry : vfs_.list(audio_dir, fs::ListOptions::FilesOnly | fs::ListOptions::Recursive)) {
                    if (!vfs_.is_directory(entry) && entry.ends_with(".bank")) {
                        auto mat_file = vfs_.materialize(entry, fs::get_special_folder(fs::SpecialFolder::Temporary));
                        if (!mat_file)
                            log(Error, "Failed to materialize: {}", mat_file->generic_string());
                        ax::AudioManager::load_bank(*mat_file);
                        log(Info, "Loaded FMOD bank: {}", entry);
                    }
                }
            } else if (project.engine_properties.flags & EngineFlags::Audio) {
                log(Warn, "Audio subsystem enabled but no associated FMod project was found");
            }
        }

        const std::string nb_module_filename = Engine::project().native_modules.front();
        if (vfs_.exists("/run/lib/" + nb_module_filename)) {
            NBMan::load(Engine::project().native_modules.front(), *active_scene_);
        } else {
            log(Error, "lib/{}: no such file or directory", nb_module_filename);
        }

        active_scene_->on_runtime_start();

        on_window_resize_event(events::WindowResizeEvent{ Engine::window().width(), Engine::window().height() });
    }

    void on_detach() override
    {
        active_scene_->on_runtime_stop();

        gfx::BatchRenderer2D::dispose();
        gfx::Renderer::dispose();
    }

    void on_update(const f32 dt) override
    {
        gfx::Renderer::clear();
        active_scene_->on_runtime_update(dt);
    }

    void on_event(events::Event& e) override
    {
        events::EventDispatcher d{ e };
        d.dispatch<events::WindowResizeEvent>(bind_event_delegate(this, &RuntimeLayer::on_window_resize_event));
    }

    bool on_window_resize_event(const events::WindowResizeEvent& e)
    {
        scene::Camera& active_camera = active_scene_->primary_camera_entity().get_component<CameraComponent>().camera;

        static ivec2 prev_viewport = ivec2{ 0, 0 };
        ivec2        resize        = ivec2{ e.width(), e.height() };
        if (prev_viewport != resize) {
            active_camera.set_width(resize.x);
            active_camera.set_height(resize.y);
            active_camera.update_projection_matrix();
            gfx::Renderer::resize_viewport(resize.x, resize.y);
            prev_viewport = resize;
            log(Info, "Viewport resize: {}", resize);
        }

        return true;
    }

private:
    fs::VirtualFilesystem&   vfs_;
    gfx::Shader              batch_shader_;
    Shared<Scene>            active_scene_;
    Box<opengl::FrameBuffer> framebuffer_;
};

class Runtime : public Engine, private Loggable<"Runtime">
{
    CX_DEFAULT_LOGGER("Runtime")

public:
    explicit Runtime(Shared<fs::VirtualFilesystem> vfs, const EngineProject& project)
        : Engine{ project }
        , vfs_{ vfs }
    {
        log(Info, "Booting project...");
        log(Info, "uuid: {}", project.uuid);
        log(Info, "name: {}", project.name);
        log(Info, "author: {}", project.author);
        log(Info, "description: {}", project.description);
        log(Info, "format_ver: {:x}", project.format_ver);
        log(Info, "engine_ver: {}", project.engine_ver.to_string());
        log(Info, "assets_root: {}", project.assets_root);
        log(Info, "config_root: {}", project.config_root);
        log(Info, "boot_scene: {}", project.boot_scene);
        log(Info, "engine_properties.video_properties.window_title: {}",
            project.engine_properties.video_properties.window_title);
        log(Info, "engine_properties.video_properties.window_width: {}",
            project.engine_properties.video_properties.window_width);
        log(Info, "engine_properties.video_properties.window_height: {}",
            project.engine_properties.video_properties.window_height);
        log(Info, "engine_properties.video_properties.window_pos_x: {}",
            project.engine_properties.video_properties.window_pos_x);
        log(Info, "engine_properties.video_properties.window_pos_y: {}",
            project.engine_properties.video_properties.window_pos_y);
        log(Info, "engine_properties.video_properties.frame_cap: {}",
            project.engine_properties.video_properties.frame_cap);
        log(Info, "engine_properties.video_properties.vsync: {}", project.engine_properties.video_properties.vsync);
        log(Info, "engine_properties.video_properties.window_flags: {:x}",
            (u32)project.engine_properties.video_properties.window_flags);
        log(Info, "engine_properties.video_properties.borderless: {:x}",
            project.engine_properties.video_properties.borderless);
        log(Info, "engine_properties.args.count: {}", project.engine_properties.args.count);
        log(Info, "engine_properties.cwd: {}", project.engine_properties.cwd.generic_string());
        log(Info, "engine_properties.flags: {:x}", (u32)project.engine_properties.flags);
        log(Info, "engine_properties.name: {}", project.engine_properties.name);
    }
    ~Runtime() override = default;

    void on_init() override { push_layer(new RuntimeLayer(*vfs_)); }

private:
    Shared<fs::VirtualFilesystem> vfs_;
};

Engine* codex::create_engine(const codex::EngineArgs args)
{
    namespace stdfs = std::filesystem;

    stdfs::path root = stdfs::current_path();
    if (args.count > 0) {
        std::error_code ec;
        auto            executable = stdfs::weakly_canonical(stdfs::absolute(args.args[0]), ec);
        if (!ec)
            root = executable.parent_path().parent_path();
    }

    auto vfs = Shared<fs::VirtualFilesystem>::make();

    auto disk_mnt = Shared<fs::DiskMount>::make(root, 0);

    auto pak_fh = disk_mnt->open("data/registry.cxpk", { fs::FileMode::Read });
    if (!pak_fh)
        throw std::runtime_error("Failed to load asset registry!");

    auto pak_mnt = Shared<fs::PakMount>::make(pak_fh, 0);
    vfs->mount(disk_mnt, "/run", true);
    vfs->mount(pak_mnt, "/run/project", true);

    EngineProject project;
    if (auto fh = vfs->open("/run/data/project.cxds", { fs::FileMode::Read }); fh) {
        std::vector<u8> buf(fh->size());
        fh->read(buf.data(), buf.size());

        BinaryArchiveBackend binsd{ buf };
        Archive              ar{ binsd };
        project.archive(ar);
    } else {
        std::cerr << "failed to open data/project.cxds" << std::endl;
        std::exit(-1);
    }

    project.engine_properties.cwd = root;
    auto defaults                 = shipped::VideoSettings::from(project.engine_properties.video_properties);
    auto settings                 = shipped::load_video_settings(root / "config/video.json", defaults);
    settings.apply(project.engine_properties.video_properties);

    return new Runtime(vfs, project);
}
