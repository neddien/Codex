#include <Codex.h>
#include <Engine/Core/EntryPoint.h>
#include <iostream>

#include "include/PlayerController.h"

using namespace codex;

class TestLayer : public Layer
{
private:
    std::unique_ptr<Scene>            m_Scene       = nullptr;
    ResRef<Shader>                    m_BatchShader = nullptr;
    std::unique_ptr<Camera>           m_Camera      = nullptr;
    std::unique_ptr<opengl::FrameBuffer> m_Framebuffer = nullptr;
    Entity                            m_Entity      = Entity::None();

public:
    void OnAttach() override
    {
        auto width    = Application::GetWindow().GetWidth();
        auto height   = Application::GetWindow().GetHeight();
        m_Scene       = std::make_unique<Scene>();
        m_BatchShader = Resources::Load<Shader>("GLShaders/batchRenderer.glsl");
        m_BatchShader->CompileShader({ { "CX_MAX_SLOT_COUNT", "16" } });
        m_Camera      = std::make_unique<Camera>(width, height);

        Renderer::Init(width, height);
        BatchRenderer2D::BindShader(m_BatchShader.get());

        opengl::FrameBufferProperties props;

        opengl::TextureProperties main;
        main.format     = opengl::TextureFormat::RGBA8;
        main.filterMode = opengl::TextureFilterMode::Nearest;

        opengl::TextureProperties id;
        id.format     = opengl::TextureFormat::RedInt32;
        id.filterMode = opengl::TextureFilterMode::Nearest;

        props.attachments.push_back(main);
        props.attachments.push_back(id);
        props.width   = Application::GetWindow().GetWidth();
        props.height  = Application::GetWindow().GetHeight();
        //m_Framebuffer = std::make_unique<opengl::FrameBuffer>(props);
        //m_Framebuffer->Unbind();

        m_Entity = m_Scene->CreateEntity();
        Sprite sp(Resources::Load<Texture2D>("Sprites/machine.png"));
        sp.SetSize({ 256, 256 });
        m_Entity.AddComponent<SpriteRendererComponent>(sp);

        auto a = m_Scene->CreateEntity();
        a.AddComponent<SpriteRendererComponent>(sp);
        a.GetComponent<TransformComponent>().position = { 700.0f, 50.0f, 0.0f };
    }
    void Update(const f32 deltaTime) override
    {
        m_BatchShader->Bind();
        m_BatchShader->SetUniformMat4f("u_View", m_Camera->GetViewMatrix());
        m_BatchShader->SetUniformMat4f("u_Proj", m_Camera->GetProjectionMatrix());

        //m_Framebuffer->Bind();
        Renderer::SetClearColour(0.2f, 0.2f, 0.2f, 1.0f);
        Renderer::Clear();
        BatchRenderer2D::Begin();
        m_Scene->Update(deltaTime);
        BatchRenderer2D::End();

        if (Input::IsMouseDown(Mouse::LeftMouse))
        {
            if (m_Entity)
            {
                Vector2f pos = { Input::GetMouseX(), Input::GetMouseY() };
                fmt::println("Selected entity at {} is {}", pos, m_Framebuffer->ReadPixel(1, pos.x, pos.y));
            }
        }
        //m_Framebuffer->Unbind();
    }
};

class TestGame : public Application
{
public:
    TestGame(const ApplicationProperties& properties) : codex::Application(properties)
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

    ~TestGame() override {}
};

Application* codex::CreateApplication(const codex::ApplicationCLIArgs args)
{
    return new TestGame(
        ApplicationProperties{ .name             = "TestGame",
                               .cwd              = "./",
                               .args             = args,
                               .windowProperties = { .width = 800, .height = 600, .frameCap = 0, .vsync = false } });
}
