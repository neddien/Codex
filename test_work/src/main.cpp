#include <codex.h>
#include <engine/core/entry_point.h>
#include <iostream>

#include "include/player_controller.h"

using namespace codex;

class TestLayer : public Layer
{
private:
    mem::Box<Scene>               scene_        = nullptr;
    ResRef<gfx::Shader>           batch_shader_ = nullptr;
    mem::Box<scene::Camera>       camera_       = nullptr;
    mem::Box<opengl::FrameBuffer> framebuffer_  = nullptr;
    Entity                        entity_       = Entity::None();

public:
    void on_attach() override
    {
        const auto width  = Engine::window().width();
        const auto height = Engine::window().height();
        scene_           = mem::Box<Scene>::make();
        batch_shader_     = Resources::load<gfx::Shader>("GLShaders/batchRenderer.glsl");
        batch_shader_->compile_shader({ { "CX_MAX_SLOT_COUNT", opengl::capabilities::max_texture_slot_count() } });
        camera_ = mem::Box<Camera>::New(width, height);

        gfx::Renderer::init(width, height);
        // gfx::BatchRenderer2D::bindShader(batch_shader_.get());

        opengl::FrameBufferProperties props;

        opengl::TextureProperties main;
        main.format     = opengl::TextureFormat::RGBA8;
        main.filterMode = opengl::TextureFilterMode::Nearest;

        opengl::TextureProperties id;
        id.format     = opengl::TextureFormat::RedInt32;
        id.filterMode = opengl::TextureFilterMode::Nearest;

        props.attachments.push_back(main);
        props.attachments.push_back(id);
        props.width  = Engine::window().width();
        props.height = Engine::window().height();
        // framebuffer_ = std::make_unique<opengl::FrameBuffer>(props);
        // framebuffer_->Unbind();

        entity_ = scene_->create_entity();
        Sprite sp(Resources::load<gfx::Texture2D>("Sprites/machine.png"));
        sp.set_size({ 256, 256 });
        entity_.add_component<SpriteRendererComponent>(sp);

        auto a = scene_->create_entity();
        a.add_component<SpriteRendererComponent>(sp);
        a.get_component<TransformComponent>().position = { 700.0f, 50.0f, 0.0f };
    }
    void on_update(const f32 deltaTime) override
    {
        batch_shader_->bind();
        batch_shader_->set_uniform_mat4f("u_View", camera_->view_matrix());
        batch_shader_->set_uniform_mat4f("u_Proj", camera_->projection_matrix());

        // framebuffer_->bind();
        gfx::Renderer::set_clear_colour(0.2f, 0.2f, 0.2f, 1.0f);
        gfx::Renderer::clear();
        gfx::BatchRenderer2D::begin();
        scene_->on_runtime_update(deltaTime);
        gfx::BatchRenderer2D::end();

        if (Input::is_mouse_down(Mouse::LeftMouse))
        {
            if (entity_)
            {
                Vector2f pos = { Input::mouse_x(), Input::mouse_y() };
                // fmt::println("Selected entity at {} is {}", pos, framebuffer_->ReadPixel(1, pos.x, pos.y));
            }
        }
        // framebuffer_->Unbind();
    }
};

class TestWork : public Engine
{
public:
    TestWork(const EngineProperties& properties)
        : codex::Engine(properties)
    {
        /*
        scene_  = (EditorScene*)window_->GetCurrentScene();
        player_ = scene_->create_entity();

        auto tex = Resources::load<codex::Texture2D>("Sprites/machine.png");

        Sprite sprite(tex);
        // f32 scale_factor = 0.05f;
        // sprite.set_texture_coords({ 0.0f, 0.0f, (f32)sprite.width() * scale_factor,
        // (f32)sprite.height() * scale_factor, });
        player_.add_component<SpriteRendererComponent>(sprite);
        auto& res = player_.get_component<TransformComponent>().scale;
        res.x     = 0.05f;
        res.y     = 0.05f;

        player_.add_component<NativeBehaviourComponent>().bind<player_controller>();
        */
        push_layer(new TestLayer());
    }

    ~TestWork() override {}
};

Engine* codex::create_engine(const codex::EngineArgs args)
{
    return new TestWork(
        EngineProperties{ .name             = "TestWork",
                          .cwd              = "./",
                          .args             = args,
                          .window_properties = { .width = 800, .height = 600, .frame_cap = 0, .vsync = false } });
}
