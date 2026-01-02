#include <Codex.h>
#include <Engine/Core/EntryPoint.h>
#include <iostream>

#include "include/PlayerController.h"

using namespace codex;

class TestLayer : public Layer
{
private:
    mem::Box<Scene>               m_Scene       = nullptr;
    ResRef<gfx::Shader>           m_BatchShader = nullptr;
    mem::Box<scene::Camera>       m_Camera      = nullptr;
    mem::Box<opengl::FrameBuffer> m_Framebuffer = nullptr;
    Entity                        m_Entity      = Entity::None();

public:
    void OnAttach() override
    {
        const auto width  = Application::GetWindow().GetWidth();
        const auto height = Application::GetWindow().GetHeight();
        m_Scene           = mem::Box<Scene>::New();
        m_BatchShader     = Resources::Load<gfx::Shader>("GLShaders/batchRenderer.glsl");
        m_BatchShader->CompileShader({ { "CX_MAX_SLOT_COUNT", opengl::capabilities::GetMaxTextureSlotCount() } });
        m_Camera = mem::Box<Camera>::New(width, height);

        gfx::Renderer::Init(width, height);
        // gfx::BatchRenderer2D::BindShader(m_BatchShader.get());

        opengl::FrameBufferProperties props;

        opengl::TextureProperties main;
        main.format     = opengl::TextureFormat::RGBA8;
        main.filterMode = opengl::TextureFilterMode::Nearest;

        opengl::TextureProperties id;
        id.format     = opengl::TextureFormat::RedInt32;
        id.filterMode = opengl::TextureFilterMode::Nearest;

        props.attachments.push_back(main);
        props.attachments.push_back(id);
        props.width  = Application::GetWindow().GetWidth();
        props.height = Application::GetWindow().GetHeight();
        // m_Framebuffer = std::make_unique<opengl::FrameBuffer>(props);
        // m_Framebuffer->Unbind();

        m_Entity = m_Scene->CreateEntity();
        Sprite sp(Resources::Load<gfx::Texture2D>("Sprites/machine.png"));
        sp.SetSize({ 256, 256 });
        m_Entity.AddComponent<SpriteRendererComponent>(sp);

        auto a = m_Scene->CreateEntity();
        a.AddComponent<SpriteRendererComponent>(sp);
        a.GetComponent<TransformComponent>().position = { 700.0f, 50.0f, 0.0f };
    }
    void OnUpdate(const f32 deltaTime) override
    {
        m_BatchShader->Bind();
        m_BatchShader->SetUniformMat4f("u_View", m_Camera->GetViewMatrix());
        m_BatchShader->SetUniformMat4f("u_Proj", m_Camera->GetProjectionMatrix());

        // m_Framebuffer->Bind();
        gfx::Renderer::SetClearColour(0.2f, 0.2f, 0.2f, 1.0f);
        gfx::Renderer::Clear();
        gfx::BatchRenderer2D::Begin();
        m_Scene->Update(deltaTime);
        gfx::BatchRenderer2D::End();

        if (Input::IsMouseDown(Mouse::LeftMouse))
        {
            if (m_Entity)
            {
                Vector2f pos = { Input::GetMouseX(), Input::GetMouseY() };
                // fmt::println("Selected entity at {} is {}", pos, m_Framebuffer->ReadPixel(1, pos.x, pos.y));
            }
        }
        // m_Framebuffer->Unbind();
    }
};

class TestWork : public Application
{
public:
    TestWork(const ApplicationProperties& properties)
        : codex::Application(properties)
    {
        /*
        m_Scene  = (EditorScene*)m_Window->GetCurrentScene();
        m_Player = m_Scene->CreateEntity();

        auto tex = Resources::Load<codex::Texture2D>("Sprites/machine.png");

        Sprite sprite(tex);
        // f32 scale_factor = 0.05f;
        // sprite.SetTextureCoords({ 0.0f, 0.0f, (f32)sprite.GetWidth() * scale_factor,
        // (f32)sprite.GetHeight() * scale_factor, });
        m_Player.AddComponent<SpriteRendererComponent>(sprite);
        auto& res = m_Player.GetComponent<TransformComponent>().scale;
        res.x     = 0.05f;
        res.y     = 0.05f;

        m_Player.AddComponent<NativeBehaviourComponent>().Bind<PlayerController>();
        */
        PushLayer(new TestLayer());
    }

    ~TestWork() override {}
};

Application* codex::CreateApplication(const codex::ApplicationCLIArgs args)
{
    return new TestWork(
        ApplicationProperties{ .name             = "TestWork",
                               .cwd              = "./",
                               .args             = args,
                               .windowProperties = { .width = 800, .height = 600, .frameCap = 0, .vsync = false } });
}
