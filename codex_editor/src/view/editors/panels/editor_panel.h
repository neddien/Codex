#pragma once

#include <codex.h>

#include "../scene_editor_view.h"

namespace codex::editor {
    class EditorPanel
    {
        friend class SceneEditorView;

    private:
        SceneEditorView&                   editor_view_;
        mem::Ref<SceneEditorDescriptor>    desc_;
        std::vector<mem::Box<EditorPanel>> view_panels_;
        bool                               imgui_block_events_ = true;

    protected:
        bool show_;
        bool focus_ = false;

    public:
        explicit EditorPanel(SceneEditorView& view) noexcept;
        virtual ~EditorPanel() noexcept;

    public:
        [[nodiscard]] inline bool imgui_block_events() const noexcept { return imgui_block_events_; }
        inline void               set_imgui_block_events(bool val) noexcept { imgui_block_events_ = val; }
        [[nodiscard]] inline mem::Ref<SceneEditorDescriptor> get_descriptor() const noexcept { return desc_; }
        [[nodiscard]] inline SceneEditorView&                get_parent() noexcept { return editor_view_; }
        [[nodiscard]] inline const SceneEditorView&          get_parent() const noexcept
        {
            return const_cast<EditorPanel*>(this)->get_parent();
        }
        inline void focus() noexcept { focus_ = true; }
        inline void close() noexcept { show_ = false; }

    protected:
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        T& attach_panel() noexcept
        {
            for (auto& e : view_panels_) {
                if (auto* ptr = dynamic_cast<T*>(e.get()); ptr != nullptr)
                    return *ptr;
            }

            view_panels_.emplace_back(new T(get_parent()));
            view_panels_.back()->on_init();
            return *static_cast<T*>(view_panels_.back().get());
        }
        template <typename T>
            requires(std::is_base_of_v<EditorPanel, T>)
        void detach_panel() noexcept
        {
            for (auto it = view_panels_.begin(); it != view_panels_.end(); ++it) {
                if (dynamic_cast<T*>(it->get()) != nullptr) {
                    view_panels_.erase(it);
                    break;
                }
            }
        }

    private:
        void         on_pre_update(const f32 deltaTime);
        virtual void on_imgui_render() = 0;
        void         on_pre_event(events::Event& e);

    protected:
        virtual void on_init() = 0;
        virtual void on_update(const f32 deltaTime) {};
        void         on_pre_imgui_render();
        virtual void on_event(events::Event& e) {}
    };
} // namespace codex::editor
