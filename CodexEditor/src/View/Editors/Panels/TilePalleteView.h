#pragma once

#include "EditorPanel.h"

namespace codex::editor {
    class TilePalleteView : public EditorPanel
    {
    public:
        using EditorPanel::EditorPanel;

    private:
        gfx::Texture2D             m_BrushIcon;
        gfx::Texture2D             m_EraseIcon;
        Entity                     m_Entity = Entity::None();
        scene::EditorCamera        m_Camera;
        Vector2f                   m_ViewportBounds[2]{};
        Vector2f                   m_ViewportSize{};
        mem::Box<opengl::FrameBuffer> m_PalleteFb = nullptr;
        Scene                      m_PalleteScene;
        Entity                     m_PalleteEntity   = Entity::None();
        bool                       m_ViewportFocused = false;
        bool                       m_ViewportHovered = false;
        gfx::DebugDraw             m_DebugDraw;
        bool                       m_PositionedPallete = false;

    public:
        [[nodiscard]] inline Entity GetEntity() const noexcept { return m_Entity; }
        void                        SetEntity(Entity newEntity) noexcept;

    protected:
        void OnInit() override;
        void OnUpdate(const f32 deltaTime) override;
        void OnImGuiRender() override;

    protected:
        // Events
        void OnEvent(events::Event& e) override;
        bool OnMouseDown_Event(events::MouseDownEvent& e);
        bool OnMouseMove_Event(events::MouseMoveEvent& e);
        bool OnMouseScroll_Event(events::MouseScrollEvent& e);
    };
} // namespace codex::editor
