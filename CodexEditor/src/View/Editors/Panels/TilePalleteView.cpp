#include "TilePalleteView.h"

#include "EditorApplication.h"

namespace codex::editor {
    void TilePalleteView::SetEntity(Entity newEntity) noexcept
    {
        if (newEntity != m_Entity)
        {
            m_Entity            = std::move(newEntity);
            m_PositionedPallete = false;
            m_Camera            = scene::EditorCamera{ 1280, 720 };
            if (m_Entity.HasComponent<TilemapComponent>())
            {
                if (!m_PalleteEntity)
                {
                    m_PalleteEntity = m_PalleteScene.CreateEntity();
                    m_PalleteEntity.AddComponent<SpriteRendererComponent>();
                    m_PalleteEntity.AddComponent<GridRendererComponent>();
                }
            }
        }
    }

    void TilePalleteView::OnInit()
    {
        const auto cwd      = EditorApplication::GetAppDataPath();
        const auto tex_spec = opengl::TextureProperties{ .format     = opengl::TextureFormat::RGBA8,
                                                      .mipmapMode = opengl::TextureMipmapMode::LinearNearest };

        m_BrushIcon = gfx::Texture2D(cwd / "Resources/brush.png", tex_spec);
        m_EraseIcon = gfx::Texture2D(cwd / "Resources/eraser.png", tex_spec);

        m_Camera = scene::EditorCamera{ 1280, 720 };

        opengl::FrameBufferProperties props;
        props.width       = 1280;
        props.height      = 720;
        props.attachments = { { .format = opengl::TextureFormat::RGBA8 } };

        // FIXME: Fix opengl::FrameBuffer's copy and move operators.
        m_PalleteFb = mem::Box<opengl::FrameBuffer>::New(props);
    }

    void TilePalleteView::OnUpdate(const f32 deltaTime)
    {
        m_PalleteFb->Bind();

        // Viewport resize
        {
            m_Camera.SetWidth(m_ViewportSize.x);
            m_Camera.SetHeight(m_ViewportSize.y);
            m_PalleteFb->Resize((u32)m_ViewportSize.x, (u32)m_ViewportSize.y);
        }

        gfx::Renderer::Clear();

        m_PalleteScene.OnEditorUpdate(deltaTime, m_Camera);

        m_DebugDraw.Begin(m_Camera);

        if (m_Entity.HasComponent<TilemapComponent>())
        {
            auto& entity_tmc        = m_Entity.GetComponent<TilemapComponent>();
            auto& pallete_grc       = m_PalleteEntity.GetComponent<GridRendererComponent>();
            auto& pallete_src       = m_PalleteEntity.GetComponent<SpriteRendererComponent>();
            pallete_src.GetSprite() = entity_tmc.sprite;
            pallete_grc.cellSize    = entity_tmc.tileSize;

            SceneEditorView::RenderGrid(m_DebugDraw, m_Camera, pallete_grc);

            if (!m_PositionedPallete)
            {
                auto pos = m_Camera.GetPos() - Vector3f{ pallete_src.GetSprite().GetSize() / 2.0f, 0.0f };
                pos      = utils::Snap(pos, Vector3f{ entity_tmc.tileSize, 1.0f });
                pos += Vector3f{ pallete_src.GetSprite().GetSize() / 2.0f, 0.0f };
                m_PalleteEntity.GetComponent<TransformComponent>().position = pos;
                m_PositionedPallete                                         = true;
            }
        }
        else
        {
            m_Entity            = Entity::None();
            m_PositionedPallete = false;
        }

        m_DebugDraw.End();

        m_PalleteFb->Unbind();
    }

    void TilePalleteView::OnImGuiRender()
    {
        static auto size = ImVec2{ 32, 32 };

        auto d = GetParent().GetDescriptor().Lock();

        if (m_Focus)
        {
            ImGui::SetNextWindowFocus();
            m_Focus = false;
        }

        if (ImGui::Begin("Tile pallete", &m_Show))
        {
            if (ImGui::ImageButton("#brush", reinterpret_cast<ImTextureID>(m_BrushIcon.GetGlId()), size, { 0, 1 },
                                   { 1, 0 }))
            {
                if (m_Entity)
                {
                    auto& c        = m_Entity.GetComponent<TilemapComponent>();
                    c.currentState = TilemapComponent::State::Brush;
                    d->selectedEntity.Select(m_Entity);
                }
            }

            ImGui::SameLine();

            if (ImGui::ImageButton("#eraser", reinterpret_cast<ImTextureID>(m_EraseIcon.GetGlId()), size, { 0, 1 },
                                   { 1, 0 }))
            {
                if (m_Entity)
                {
                    auto& c        = m_Entity.GetComponent<TilemapComponent>();
                    c.currentState = TilemapComponent::State::Erase;
                    d->selectedEntity.Select(m_Entity);
                }
            }

            if (m_Entity)
            {
                auto& c = m_Entity.GetComponent<TilemapComponent>();
                if (c.sprite)
                {
                    const auto viewport_min_region = ImGui::GetWindowContentRegionMin();
                    const auto viewport_max_region = ImGui::GetWindowContentRegionMax();
                    const auto viewport_offset     = ImGui::GetWindowPos();
                    m_ViewportBounds[0]            = { viewport_min_region.x + viewport_offset.x,
                                                       viewport_min_region.y + viewport_offset.y };
                    m_ViewportBounds[1]            = { viewport_max_region.x + viewport_offset.x,
                                                       viewport_max_region.y + viewport_offset.y };

                    auto current_viewport_window_size = ImGui::GetContentRegionAvail();
                    m_ViewportSize = Vector2f{ current_viewport_window_size.x, current_viewport_window_size.y };
                    ImGui::Image(reinterpret_cast<ImTextureID>(m_PalleteFb->GetColourAttachmentIdAt(0)),
                                 current_viewport_window_size, { 0, 1 }, { 1, 0 });

                    m_ViewportFocused = ImGui::IsWindowFocused();
                    m_ViewportHovered = ImGui::IsWindowHovered();

                    SetImGuiBlockEvents(!m_ViewportFocused);
                }
            }

            if (!m_Show)
                this->Close();
        }
        ImGui::End();
    }

    void TilePalleteView::OnEvent(events::Event& e)
    {
        events::EventDispatcher d{ e };
        d.Dispatch<events::MouseDownEvent>(BindEventDelegate(this, &TilePalleteView::OnMouseDown_Event));
        d.Dispatch<events::MouseMoveEvent>(BindEventDelegate(this, &TilePalleteView::OnMouseMove_Event));
        d.Dispatch<events::MouseScrollEvent>(BindEventDelegate(this, &TilePalleteView::OnMouseScroll_Event));
    }

    bool TilePalleteView::OnMouseDown_Event(events::MouseDownEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;

        if (m_Entity && mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x &&
            mouse_pos.y <= m_ViewportSize.y)
        {
            auto& c = m_Entity.GetComponent<TilemapComponent>();

            const auto mouse_in_scene =
                utils::Snap(scene::Camera::ScreenCoordinatesToWorld(m_Camera, mouse_pos, m_Camera.GetPos()),
                            Vector3f{ c.tileSize, 1.0f });
            const auto pallete_entity_size =
                m_PalleteEntity.GetComponent<SpriteRendererComponent>().GetSprite().GetSize();
            const auto pallete_entity_top_left = m_PalleteEntity.GetComponent<TransformComponent>().position -
                                                 utils::ToVec3f(pallete_entity_size / 2.0f);
            if (mouse_in_scene.x >= pallete_entity_top_left.x && mouse_in_scene.y >= pallete_entity_top_left.y &&
                mouse_in_scene.x <= pallete_entity_top_left.x + pallete_entity_size.x &&
                mouse_in_scene.y <= pallete_entity_top_left.y + pallete_entity_size.y)
            {
                c.currentTile = Vector2f{ mouse_in_scene } - Vector2f{ pallete_entity_top_left };
            }
            return true;
        }
        return false;
    }

    bool TilePalleteView::OnMouseMove_Event(events::MouseMoveEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x && mouse_pos.y <= m_ViewportSize.y)
        {
            if (Input::IsMouseDown(Mouse::MiddleMouse))
            {
                if (Input::IsMouseDragging())
                {
                    const auto vec = Vector2f{ Input::GetMouseDeltaX(), Input::GetMouseDeltaY() * -1.0f };
                    m_Camera.SetPos(m_Camera.GetPos() + utils::ToVec3f(vec) * m_Camera.GetPan());
                }
                return true;
            }
        }
        return false;
    }

    bool TilePalleteView::OnMouseScroll_Event(events::MouseScrollEvent& e)
    {
        auto mouse_pos = Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        mouse_pos.x -= m_ViewportBounds[0].x;
        mouse_pos.y -= m_ViewportBounds[0].y;
        mouse_pos.y = (m_ViewportBounds[1] - m_ViewportBounds[0]).y - mouse_pos.y;
        if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x <= m_ViewportSize.x && mouse_pos.y <= m_ViewportSize.y)
        {
            m_Camera.SetPan(m_Camera.GetPan() + e.GetOffsetY() * -0.05f);
            return true;
        }
        return false;
    }
} // namespace codex::editor
