#include <codex.h>
#include <engine/core/entry_point.h>

using namespace codex;

class TestLayer : public Layer, public Loggable<"TestLayer">
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

        camera_ = scene::Camera(width, height);

        auto fh = vfs_.open("/assets/blue_pascal.png");
        if (fh) {
            std::vector<u8> buf(fh->size());
            fh->read(buf.data(), buf.size());

            texture_ = gfx::Texture2D{ buf.data(), buf.size() };

            log(Info, "Loaded texture: {}", fh->path());
        }

        sprite_scale_           = { 256.0f * 2, 256.0f * 2, 1.0f };
        sprite_transform_.scale = sprite_scale_;
        window_orig_size_       = { Engine::window().width(), Engine::window().height() };
    }

    void on_detach() override
    {
        gfx::BatchRenderer2D::dispose();
        gfx::Renderer::dispose();
    }

    void on_update(const f32 delta_time) override
    {
        // Keep sprite centred at the camera focal point.
        sprite_transform_.position = camera_transform_.position;

        gfx::Renderer::clear();
        gfx::BatchRenderer2D::begin(camera_, camera_transform_);

        if (texture_) {
            gfx::BatchRenderer2D::render_rect(
                &texture_, opengl::Rectf{ 0.0f, 0.0f, (f32)texture_.width(), (f32)texture_.height() },
                sprite_transform_.to_matrix(), Vector4f{ 1.0f, 1.0f, 1.0f, 1.0f });
        }

        gfx::BatchRenderer2D::end();

        const auto title =
            fmt::format("TestWork: Pascal Demo @ {}fps, {}ms", static_cast<u32>(1.0f / delta_time), delta_time);
        Engine::window().set_title(title.c_str());
    }

    void on_event(events::Event& e) override
    {
        events::EventDispatcher d{ e };
        d.dispatch<events::WindowResizeEvent>(
            [this](const events::WindowResizeEvent& ev)
            {
                camera_.set_width(ev.width());
                camera_.set_height(ev.height());

                const auto scaler       = std::min(ev.width() / window_orig_size_.x, ev.height() / window_orig_size_.y);
                sprite_transform_.scale = Vector3f{ sprite_scale_.x * scaler, sprite_scale_.y * scaler, 1.0f };

                return true;
            });
    }

private:
    fs::VirtualFilesystem vfs_;
    scene::Camera         camera_;
    TransformComponent    camera_transform_;
    gfx::Texture2D        texture_;
    Vector2f              window_orig_size_;
    Vector3f              sprite_scale_;
    TransformComponent    sprite_transform_;
};

class TestWork : public Engine
{
public:
    explicit TestWork(const EngineProperties& properties)
        : Engine(properties)
    {
        push_layer(new TestLayer());
    }
    ~TestWork() override = default;
};

Engine* codex::create_engine(const codex::EngineArgs args)
{
    return new TestWork(
        EngineProperties{ .name              = "TestWork",
                          .cwd               = "./",
                          .args              = args,
                          .flags             = codex::EngineFlags::Video | codex::EngineFlags::Logger,
                          .window_properties = { .width = 800, .height = 600, .frame_cap = 300, .vsync = false } });
}
