#include <codex.h>
#include <engine/core/entry_point.h>

using namespace codex;

class RuntimeLayer : public Layer, public Loggable<"TestLayer">
{
public:
    void on_attach() override
    {
        const auto width  = Engine::window().width();
        const auto height = Engine::window().height();

        vfs_.mount(Shared<fs::DiskMount>::make(std::filesystem::current_path(), 0), "/assets", true);
        log(Info, "VFS: Mounted /assets as {}", std::filesystem::current_path().generic_string());

        gfx::Renderer::init(width, height);
        gfx::BatchRenderer2D::init(vfs_, "/assets/batch_renderer2d_quad.glsl");
        gfx::Renderer::set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);
    }

    void on_detach() override
    {
        gfx::BatchRenderer2D::dispose();
        gfx::Renderer::dispose();
    }

    void on_update(const f32 delta_time) override
    {
        gfx::Renderer::clear();
        // gfx::BatchRenderer2D::begin(camera_, camera_transform_);

        // exec scene

        // gfx::BatchRenderer2D::end();
    }

    void on_event(events::Event& e) override
    {
        // no need for events for now
    }

private:
    fs::VirtualFilesystem vfs_;
};

class Runtime : public Engine
{
public:
    explicit Runtime(const EngineProject& project)
        : Engine{ project }
    {
        push_layer(new RuntimeLayer());
    }
    ~Runtime() override = default;
};

Engine* codex::create_engine(const codex::EngineArgs args)
{
    Shared<fs::VirtualFilesystem> vfs;

    auto disk_mnt = Shared<fs::DiskMount>::make("./", 0);

    auto pak_fh = disk_mnt->open("asset_registry.cxpkz", { fs::FileMode::Read });
    if (!pak_fh)
        throw std::runtime_error("Failed to load asset registry!");

    auto pak_mnt = Shared<fs::PakMount>::make(pak_fh, 0);
    vfs->mount(disk_mnt, "/run", true);
    vfs->mount(pak_mnt, "/run/project/assets", true);

    for (const auto& e : vfs->list("/run/project/assets", fs::ListOptions::Recursive)) {
        std::cout << e << std::endl;
    }

    std::exit(0);

    return nullptr;
    // return new Runtime(
    //     EngineProperties{ .name              = "TestWork",
    //                       .cwd               = "./",
    //                       .args              = args,
    //                       .flags             = codex::EngineFlags::Video | codex::EngineFlags::Logger,
    //                       .window_properties = { .width = 800, .height = 600, .frame_cap = 300, .vsync = false } });
}
